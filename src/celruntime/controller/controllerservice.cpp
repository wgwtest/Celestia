// controllerservice.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "controllerservice.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/viewinput.h>

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

std::string
serializeModelInputCommand(const protocol::ViewInputEvent& input)
{
    return "action=" + input.action +
           ";key=" + input.key +
           ";pointerX=" + std::to_string(input.pointer[0]) +
           ";pointerY=" + std::to_string(input.pointer[1]) +
           ";wheelX=" + std::to_string(input.wheel[0]) +
           ";wheelY=" + std::to_string(input.wheel[1]) +
           ";modifiers=" + input.modifiers;
}

bool
hasModifier(std::string_view modifiers, std::string_view expected)
{
    return modifiers.find(expected) != std::string_view::npos;
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
    {
        const auto fields = parsePayload(request.payload);
        const auto view = fields.find("view");
        if (view != fields.end() && view->second == "celestia.view3d.opengl")
        {
            const auto dt = fields.find("dt");
            auto payload = std::string("dt=") + (dt == fields.end() ? "0.125" : dt->second);
            payload += ";view=celestia.view3d.opengl";
            return { commandToModel(request, "model.step", std::move(payload)) };
        }

        return { commandToModel(request, "model.requestSnapshot") };
    }

    if (request.kind != RuntimeMessageKind::Event)
        return { errorResponse(request, "controller service expects input events or controller commands") };

    if (request.name == protocol::ViewInputMessageName)
    {
        const auto input = protocol::deserializeViewInputEvent(request.payload);
        if (!input.has_value())
            return { errorResponse(request, "invalid view.input payload") };

        if (input->action == "Quit")
        {
            auto shutdown = makeEnvelope(request, RuntimeRole::Broadcast, RuntimeMessageKind::Lifecycle,
                                         protocol::RuntimeShutdown);
            shutdown.sequenceId = request.sequenceId;
            return { shutdown };
        }

        if (input->device == "keyboard" && input->action == "KeyDown" && input->key == "Space")
        {
            paused_ = !paused_;
            auto payload = std::string("paused=") + (paused_ ? "true" : "false");
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=time.pause";
            return { commandToModel(request, "model.setPaused", std::move(payload)) };
        }

        if (input->device == "keyboard" && input->action == "KeyDown" && input->key == "L")
        {
            timeScale_ *= 2.0;
            auto payload = std::string("timeScale=") + std::to_string(timeScale_);
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=time.setScale";
            return { commandToModel(request, "model.setTimeScale", std::move(payload)) };
        }

        if (input->device == "keyboard" &&
            input->action == "KeyDown" &&
            input->key == "Backspace" &&
            hasModifier(input->modifiers, "Ctrl"))
        {
            auto payload = std::string("view=celestia.view3d.opengl");
            payload += ";command=selection.clear";
            return { commandToModel(request, "model.clearSelection", std::move(payload)) };
        }

        if (input->device == "keyboard" && input->action == "KeyDown" && input->key == "H")
        {
            auto payload = std::string("type=star");
            payload += ";id=celestia:star:Sol";
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=selection.selectObject";
            return { commandToModel(request, "model.setSelection", std::move(payload)) };
        }

        if (input->device == "keyboard" && input->action == "KeyDown" && input->key == "C")
        {
            auto payload = std::string("target=currentSelection");
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=camera.center";
            return { commandToModel(request, "model.centerSelection", std::move(payload)) };
        }

        if (input->device == "keyboard" &&
            input->action == "KeyDown" &&
            (input->key == "Left" || input->key == "Right"))
        {
            const auto yawDegrees = input->key == "Left" ? -15.0 : 15.0;
            auto payload = std::string("target=currentSelection");
            payload += ";yawDegrees=" + std::to_string(yawDegrees);
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=camera.orbit";
            return { commandToModel(request, "model.orbitCamera", std::move(payload)) };
        }

        if (input->device == "mouse" && input->action == "MouseWheel" && input->wheel[1] != 0.0)
        {
            cameraFov_ = input->wheel[1] > 0.0
                ? std::max(5.0, cameraFov_ - 5.0)
                : std::min(120.0, cameraFov_ + 5.0);
            auto payload = std::string("fov=") + std::to_string(cameraFov_);
            payload += ";view=celestia.view3d.opengl";
            payload += ";command=camera.zoom";
            return { commandToModel(request, "model.setCameraFov", std::move(payload)) };
        }

        return { commandToModel(request, "model.setViewInput", serializeModelInputCommand(*input)) };
    }

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
