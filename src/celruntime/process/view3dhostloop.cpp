// view3dhostloop.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dhostloop.h"

#include <ostream>
#include <utility>
#include <vector>

#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>
#include <celruntime/transport/stdiotransport.h>
#include <celruntime/view3d/view3dhost.h>

namespace celestia::runtime::process
{
namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

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

View3DHostCallbacks&
callbacks()
{
    static View3DHostCallbacks registered;
    return registered;
}

bool
sendAll(transport::FramedTransport& transport, const std::vector<RuntimeEnvelope>& messages)
{
    for (const auto& message : messages)
    {
        if (!transport.send(message))
            return false;
    }

    return true;
}

bool
sendViewInputEvents(transport::FramedTransport& transport,
                    std::vector<protocol::ViewInputEvent> events,
                    std::string_view sessionId)
{
    for (auto& event : events)
    {
        if (event.sessionId.empty())
            event.sessionId = std::string(sessionId);
        if (!transport.send(protocol::viewInputEnvelope(event, RuntimeRole::View, RuntimeRole::Controller)))
            return false;
    }

    return true;
}

} // end unnamed namespace

void
setRuntimeView3DHostCallbacks(View3DHostCallbacks callbacks)
{
    process::callbacks() = callbacks;
}

int
runRuntimeView3DHostLoop(std::string sessionId,
                         std::istream& input,
                         std::ostream& output,
                         std::ostream& error)
{
    transport::StdioTransport transport(input, output);
    view3d::View3DHost viewHost(sessionId);

    for (;;)
    {
        auto received = transport.receive();
        if (received.status == transport::ReceiveStatus::Closed)
            return 0;

        if (received.status == transport::ReceiveStatus::Malformed ||
            received.status == transport::ReceiveStatus::WouldBlock ||
            !received.message.has_value())
        {
            error << "invalid framed runtime message\n";
            return 2;
        }

        auto request = *received.message;
        if (request.sessionId.empty())
            request.sessionId = sessionId;

        if (request.kind == RuntimeMessageKind::Lifecycle)
        {
            if (request.name == protocol::RuntimeHello)
            {
                if (!transport.send(makeResponse(request,
                                                 RuntimeMessageKind::Lifecycle,
                                                 protocol::RuntimeReady)))
                {
                    return 2;
                }
                continue;
            }

            if (request.name == protocol::RuntimeHeartbeat)
            {
                if (!transport.send(makeResponse(request,
                                                 RuntimeMessageKind::Heartbeat,
                                                 protocol::RuntimeHeartbeat)))
                {
                    return 2;
                }
                continue;
            }

            if (request.name == protocol::RuntimeStart ||
                request.name == protocol::RuntimeShutdown)
            {
                if (request.name == protocol::RuntimeStart && callbacks().start != nullptr)
                {
                    std::string errorMessage;
                    if (!callbacks().start(errorMessage))
                    {
                        transport.send(makeResponse(request,
                                                    RuntimeMessageKind::Error,
                                                    protocol::RuntimeError,
                                                    errorMessage.empty()
                                                        ? "failed to start view3d renderer"
                                                        : std::move(errorMessage)));
                        return 2;
                    }
                }

                if (!sendAll(transport, viewHost.handle(request)))
                    return 2;
                if (request.name == protocol::RuntimeShutdown)
                {
                    if (callbacks().stop != nullptr)
                        callbacks().stop();
                    return 0;
                }
                continue;
            }
        }

        if (request.kind == RuntimeMessageKind::ViewFrame)
        {
            if (request.name == protocol::SceneFrameMessageName && callbacks().render != nullptr)
            {
                const auto frame = protocol::deserializeSceneFrame(request.payload);
                if (!frame.has_value())
                {
                    if (!transport.send(makeResponse(request,
                                                     RuntimeMessageKind::Error,
                                                     protocol::RuntimeError,
                                                     "invalid scene.frame payload")))
                    {
                        return 2;
                    }
                    continue;
                }

                std::string errorMessage;
                if (!callbacks().render(*frame, errorMessage))
                {
                    if (!transport.send(makeResponse(request,
                                                     RuntimeMessageKind::Error,
                                                     protocol::RuntimeError,
                                                     errorMessage.empty()
                                                         ? "failed to render scene.frame"
                                                         : std::move(errorMessage))))
                    {
                        return 2;
                    }
                    continue;
                }
            }

            if (!sendAll(transport, viewHost.handle(request)))
                return 2;
            if (callbacks().drainInput != nullptr)
            {
                if (!sendViewInputEvents(transport, callbacks().drainInput(), request.sessionId))
                    return 2;
            }
            continue;
        }

        if (request.kind == RuntimeMessageKind::Command ||
            request.kind == RuntimeMessageKind::Event)
        {
            if (!sendAll(transport, viewHost.handle(request)))
                return 2;
            continue;
        }

        if (!transport.send(makeResponse(request,
                                         RuntimeMessageKind::Error,
                                         protocol::RuntimeError,
                                         "expected view3d lifecycle or scene.frame message")))
        {
            return 2;
        }
    }
}

} // namespace celestia::runtime::process
