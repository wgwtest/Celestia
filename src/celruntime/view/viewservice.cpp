// viewservice.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "viewservice.h"

#include <string>
#include <utility>
#include <vector>

#include <celruntime/protocol/lifecycle.h>
#include <celruntime/viewframecodec.h>

namespace celestia::runtime::view
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

RuntimeEnvelope
makeResponse(const RuntimeEnvelope& request,
             RuntimeMessageKind kind,
             std::string name,
             std::string payload = {})
{
    RuntimeEnvelope response;
    response.sessionId = request.sessionId;
    response.sequenceId = request.sequenceId;
    response.sourceRole = RuntimeRole::View;
    response.targetRole = request.sourceRole;
    response.kind = kind;
    response.name = std::move(name);
    response.payload = std::move(payload);
    return response;
}

} // end unnamed namespace

ViewService::ViewService(std::string sessionId)
    : sessionId_(std::move(sessionId))
{
}

bool
ViewService::isRunning() const
{
    return running_;
}

std::uint64_t
ViewService::lastFrameId() const
{
    return lastFrameId_;
}

double
ViewService::lastSimulationTime() const
{
    return lastSimulationTime_;
}

std::string_view
ViewService::lastFrameSummary() const
{
    return lastFrameSummary_;
}

std::uint64_t
ViewService::frameCount() const
{
    return frameCount_;
}

RuntimeEnvelope
ViewService::lifecycleResponse(const RuntimeEnvelope& request, std::string name) const
{
    return makeResponse(request, RuntimeMessageKind::Lifecycle, std::move(name));
}

RuntimeEnvelope
ViewService::errorResponse(const RuntimeEnvelope& request, std::string message) const
{
    return makeResponse(request, RuntimeMessageKind::Error, protocol::RuntimeError, std::move(message));
}

std::vector<RuntimeEnvelope>
ViewService::handle(const RuntimeEnvelope& request)
{
    if (request.kind == RuntimeMessageKind::Lifecycle)
    {
        if (request.name == protocol::RuntimeStart)
        {
            running_ = true;
            return { lifecycleResponse(request, "view.running") };
        }

        if (request.name == protocol::RuntimeShutdown)
        {
            running_ = false;
            return { lifecycleResponse(request, protocol::RuntimeStopped) };
        }

        return { errorResponse(request, "unknown lifecycle message: " + request.name) };
    }

    if (request.kind != RuntimeMessageKind::ViewFrame || request.name != "view.frame")
        return { errorResponse(request, "view service expects view.frame messages") };

    const auto frame = deserializeViewFrame(request.payload);
    if (!frame.has_value())
        return { errorResponse(request, "invalid view.frame payload") };

    lastFrameId_ = frame->frameId;
    lastSimulationTime_ = frame->time;
    lastFrameSummary_ = frame->summary;
    ++frameCount_;
    return {};
}

RuntimeEnvelope
ViewService::closeRequested() const
{
    RuntimeEnvelope envelope;
    envelope.sessionId = sessionId_;
    envelope.sourceRole = RuntimeRole::View;
    envelope.targetRole = RuntimeRole::Controller;
    envelope.kind = RuntimeMessageKind::Event;
    envelope.name = "input.closeRequested";
    return envelope;
}

} // namespace celestia::runtime::view
