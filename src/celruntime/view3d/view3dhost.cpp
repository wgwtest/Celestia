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
                     const View3DSceneState& state)
{
    std::ostringstream output;
    output << "frameCount=" << frameCount
           << ";lastSequence=" << state.sequence
           << ";simulationTime=" << state.simulationTime
           << ";bodyCount=" << state.bodyCount
           << ";starCount=" << state.starCount
           << ";deepSkyObjectCount=" << state.deepSkyObjectCount
           << ";orbitCount=" << state.orbitCount
           << ";labelCount=" << state.labelCount
           << ";resourceCount=" << state.resourceCount
           << ";resolvedResourceCount=" << state.resolvedResourceCount
           << ";missingRequiredResourceCount=" << state.missingRequiredResourceCount
           << ";cameraFov=" << state.cameraFov;
    return output.str();
}

} // end unnamed namespace

View3DHost::View3DHost(std::string sessionId, View3DHostOptions options)
    : sessionId_(std::move(sessionId))
    , options_(std::move(options))
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
    return lastSceneState_.sequence;
}

double
View3DHost::lastSimulationTime() const
{
    return lastSceneState_.simulationTime;
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
                    "renderer=step15-scene-consumer;capabilities=scene.frame,view.input,opengl,resources;frames=0");
}

RuntimeEnvelope
View3DHost::frameRendered(const RuntimeEnvelope& request) const
{
    return response(request,
                    RuntimeMessageKind::Event,
                    "view.frameRendered",
                    frameRenderedPayload(frameCount_, lastSceneState_));
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

    lastSceneState_ = buildView3DSceneState(*frame, options_.contentRoot);
    ++frameCount_;
    return { frameRendered(request) };
}

} // namespace celestia::runtime::view3d
