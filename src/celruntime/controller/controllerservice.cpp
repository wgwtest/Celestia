// controllerservice.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "controllerservice.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <celruntime/protocol/lifecycle.h>

namespace celestia::runtime::controller
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

std::unordered_map<std::string, std::string>
parsePayload(std::string_view payload)
{
    std::unordered_map<std::string, std::string> fields;
    std::size_t offset = 0;

    while (offset <= payload.size())
    {
        auto end = payload.find(';', offset);
        if (end == std::string_view::npos)
            end = payload.size();

        const auto part = payload.substr(offset, end - offset);
        const auto separator = part.find('=');
        if (separator != std::string_view::npos)
            fields[std::string(part.substr(0, separator))] = std::string(part.substr(separator + 1));

        if (end == payload.size())
            break;
        offset = end + 1;
    }

    return fields;
}

RuntimeEnvelope
makeEnvelope(const RuntimeEnvelope& request,
             RuntimeRole target,
             RuntimeMessageKind kind,
             std::string name,
             std::string payload = {})
{
    RuntimeEnvelope response;
    response.sessionId = request.sessionId;
    response.sequenceId = request.sequenceId;
    response.sourceRole = RuntimeRole::Controller;
    response.targetRole = target;
    response.kind = kind;
    response.name = std::move(name);
    response.payload = std::move(payload);
    return response;
}

} // end unnamed namespace

ControllerService::ControllerService(std::string sessionId)
    : sessionId_(std::move(sessionId))
{
}

bool
ControllerService::isRunning() const
{
    return running_;
}

RuntimeEnvelope
ControllerService::commandToModel(const RuntimeEnvelope& request,
                                  std::string name,
                                  std::string payload) const
{
    return makeEnvelope(request, RuntimeRole::Model, RuntimeMessageKind::Command,
                        std::move(name), std::move(payload));
}

RuntimeEnvelope
ControllerService::lifecycleToLauncher(const RuntimeEnvelope& request, std::string name) const
{
    return makeEnvelope(request, request.sourceRole, RuntimeMessageKind::Lifecycle, std::move(name));
}

RuntimeEnvelope
ControllerService::errorResponse(const RuntimeEnvelope& request, std::string message) const
{
    return makeEnvelope(request, request.sourceRole, RuntimeMessageKind::Error,
                        protocol::RuntimeError, std::move(message));
}

std::vector<RuntimeEnvelope>
ControllerService::handle(const RuntimeEnvelope& request)
{
    if (request.kind == RuntimeMessageKind::Lifecycle)
    {
        if (request.name == protocol::RuntimeStart)
        {
            running_ = true;
            return { lifecycleToLauncher(request, "controller.running") };
        }

        if (request.name == protocol::RuntimeShutdown)
        {
            running_ = false;
            return { lifecycleToLauncher(request, protocol::RuntimeStopped) };
        }

        return { errorResponse(request, "unknown lifecycle message: " + request.name) };
    }

    if (request.kind == RuntimeMessageKind::Command && request.name == "controller.tick")
        return { commandToModel(request, "model.requestSnapshot") };

    if (request.kind != RuntimeMessageKind::Event)
        return { errorResponse(request, "controller service expects input events or controller commands") };

    if (request.name == "input.key")
    {
        const auto fields = parsePayload(request.payload);
        const auto key = fields.find("key");
        if (key != fields.end() && key->second == "Space")
        {
            paused_ = !paused_;
            return { commandToModel(request, "model.setPaused",
                                    std::string("paused=") + (paused_ ? "true" : "false")) };
        }

        return { errorResponse(request, "unsupported key input") };
    }

    if (request.name == "input.closeRequested")
    {
        auto shutdown = makeEnvelope(request, RuntimeRole::Broadcast, RuntimeMessageKind::Lifecycle,
                                     protocol::RuntimeShutdown);
        shutdown.sequenceId = request.sequenceId;
        return { shutdown };
    }

    return { errorResponse(request, "unknown input event: " + request.name) };
}

} // namespace celestia::runtime::controller
