// runtimehostloop.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimehostloop.h"

#include <ostream>
#include <utility>
#include <vector>

#include <celruntime/controller/controllerservice.h>
#include <celruntime/model/modelservice.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/transport/stdiotransport.h>
#include <celruntime/view/viewservice.h>

namespace celestia::runtime::process
{
namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

RuntimeEnvelope
makeResponse(const RuntimeEnvelope& request,
             RuntimeRole source,
             RuntimeMessageKind kind,
             std::string name,
             std::string payload = {})
{
    RuntimeEnvelope response;
    response.sessionId = request.sessionId;
    response.sequenceId = request.sequenceId;
    response.sourceRole = source;
    response.targetRole = request.sourceRole;
    response.kind = kind;
    response.name = std::move(name);
    response.payload = std::move(payload);
    return response;
}

RuntimeEnvelope
handleLifecycle(const RuntimeEnvelope& request, RuntimeRole role)
{
    if (request.name == protocol::RuntimeHello)
        return makeResponse(request, role, RuntimeMessageKind::Lifecycle, protocol::RuntimeReady);

    if (request.name == protocol::RuntimeHeartbeat)
        return makeResponse(request, role, RuntimeMessageKind::Heartbeat, protocol::RuntimeHeartbeat);

    if (request.name == protocol::RuntimeShutdown)
        return makeResponse(request, role, RuntimeMessageKind::Lifecycle, protocol::RuntimeStopped);

    if (request.name == protocol::RuntimeStart)
        return makeResponse(request, role, RuntimeMessageKind::Lifecycle, "runtime.started");

    return makeResponse(request, role, RuntimeMessageKind::Error, protocol::RuntimeError,
                        "unknown lifecycle message: " + request.name);
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

} // end unnamed namespace

std::optional<RuntimeRole>
runtimeRoleFromHostRole(std::string_view role)
{
    if (role == "model")
        return RuntimeRole::Model;
    if (role == "controller")
        return RuntimeRole::Controller;
    if (role == "view")
        return RuntimeRole::View;
    return std::nullopt;
}

int
runRuntimeHostLoop(RuntimeRole role,
                   std::string sessionId,
                   std::istream& input,
                   std::ostream& output,
                   std::ostream& error)
{
    transport::StdioTransport transport(input, output);
    return runRuntimeHostLoop(role, std::move(sessionId), transport, error);
}

int
runRuntimeHostLoop(RuntimeRole role,
                   std::string sessionId,
                   transport::FramedTransport& transport,
                   std::ostream& error)
{
    model::ModelService modelService(sessionId);
    controller::ControllerService controllerService(sessionId);
    view::ViewService viewService(sessionId);

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

        if (request.name == protocol::RuntimeStart && request.kind == RuntimeMessageKind::Lifecycle)
        {
            if (role == RuntimeRole::Model)
            {
                if (!transport.send(modelService.handle(request)))
                    return 2;
                continue;
            }

            if (role == RuntimeRole::Controller)
            {
                if (!sendAll(transport, controllerService.handle(request)))
                    return 2;
                continue;
            }

            if (role == RuntimeRole::View)
            {
                if (!sendAll(transport, viewService.handle(request)))
                    return 2;
                continue;
            }
        }

        if (request.kind == RuntimeMessageKind::Command ||
            request.kind == RuntimeMessageKind::Event ||
            request.kind == RuntimeMessageKind::ViewFrame)
        {
            if (role == RuntimeRole::Model)
            {
                if (!transport.send(modelService.handle(request)))
                    return 2;
                continue;
            }

            if (role == RuntimeRole::Controller)
            {
                if (!sendAll(transport, controllerService.handle(request)))
                    return 2;
                continue;
            }

            if (role == RuntimeRole::View)
            {
                if (!sendAll(transport, viewService.handle(request)))
                    return 2;
                continue;
            }
        }

        if (request.kind != RuntimeMessageKind::Lifecycle &&
            request.kind != RuntimeMessageKind::Heartbeat)
        {
            const auto response = makeResponse(request, role, RuntimeMessageKind::Error,
                                               protocol::RuntimeError,
                                               "expected lifecycle message");
            transport.send(response);
            continue;
        }

        const auto response = handleLifecycle(request, role);
        if (!transport.send(response))
            return 2;

        if (request.name == protocol::RuntimeShutdown)
            return 0;
    }
}

} // namespace celestia::runtime::process
