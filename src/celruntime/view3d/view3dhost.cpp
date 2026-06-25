// view3dhost.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dhost.h"

#include <sstream>
#include <string>
#include <utility>

#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>

namespace celestia::runtime::view3d
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

std::string
frameRenderedPayload(std::uint64_t frameCount,
                     std::uint64_t lastSequence,
                     double lastSimulationTime,
                     std::uint64_t bodyCount,
                     std::uint64_t starCount)
{
    std::ostringstream output;
    output << "frameCount=" << frameCount
           << ";lastSequence=" << lastSequence
           << ";simulationTime=" << lastSimulationTime
           << ";bodyCount=" << bodyCount
           << ";starCount=" << starCount;
    return output.str();
}

} // end unnamed namespace

View3DHost::View3DHost(std::string sessionId)
    : sessionId_(std::move(sessionId))
{
}

bool
View3DHost::isRunning() const
{
    return running_;
}

std::uint64_t
View3DHost::frameCount() const
{
    return frameCount_;
}

std::uint64_t
View3DHost::lastSequence() const
{
    return lastSequence_;
}

double
View3DHost::lastSimulationTime() const
{
    return lastSimulationTime_;
}

RuntimeEnvelope
View3DHost::response(const RuntimeEnvelope& request,
                     RuntimeMessageKind kind,
                     std::string name,
                     std::string payload) const
{
    RuntimeEnvelope envelope;
    envelope.sessionId = request.sessionId.empty() ? sessionId_ : request.sessionId;
    envelope.sequenceId = request.sequenceId;
    envelope.sourceRole = RuntimeRole::View;
    envelope.targetRole = request.sourceRole;
    envelope.kind = kind;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

RuntimeEnvelope
View3DHost::errorResponse(const RuntimeEnvelope& request, std::string message) const
{
    return response(request, RuntimeMessageKind::Error, protocol::RuntimeError, std::move(message));
}

RuntimeEnvelope
View3DHost::ready3D(const RuntimeEnvelope& request) const
{
    return response(request,
                    RuntimeMessageKind::Event,
                    "view.ready3d",
                    "renderer=step8-scene-protocol;capabilities=scene.frame,view.input,opengl;frames=0");
}

RuntimeEnvelope
View3DHost::frameRendered(const RuntimeEnvelope& request) const
{
    return response(request,
                    RuntimeMessageKind::Event,
                    "view.frameRendered",
                    frameRenderedPayload(frameCount_,
                                         lastSequence_,
                                         lastSimulationTime_,
                                         lastBodyCount_,
                                         lastStarCount_));
}

std::vector<RuntimeEnvelope>
View3DHost::handle(const RuntimeEnvelope& request)
{
    if (request.kind == RuntimeMessageKind::Lifecycle)
    {
        if (request.name == protocol::RuntimeStart)
        {
            running_ = true;
            return { ready3D(request) };
        }

        if (request.name == protocol::RuntimeShutdown)
        {
            running_ = false;
            return { response(request, RuntimeMessageKind::Lifecycle, protocol::RuntimeStopped) };
        }

        return { errorResponse(request, "unknown view3d lifecycle message: " + request.name) };
    }

    if (request.kind != RuntimeMessageKind::ViewFrame ||
        request.name != protocol::SceneFrameMessageName)
    {
        return { errorResponse(request, "view3d host expects scene.frame messages") };
    }

    const auto frame = protocol::deserializeSceneFrame(request.payload);
    if (!frame.has_value())
        return { errorResponse(request, "invalid scene.frame payload") };

    lastSequence_ = frame->sequence;
    lastSimulationTime_ = frame->simulationTime;
    lastBodyCount_ = static_cast<std::uint64_t>(frame->bodies.size());
    lastStarCount_ = static_cast<std::uint64_t>(frame->stars.size());
    ++frameCount_;
    return { frameRendered(request) };
}

} // namespace celestia::runtime::view3d
