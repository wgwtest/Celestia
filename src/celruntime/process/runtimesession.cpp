// runtimesession.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimesession.h"

#include "hostprocess.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/protocol/viewinput.h>
#include <celruntime/runtimeconfig.h>

namespace celestia::runtime::process
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

struct RuntimeHost
{
    RuntimeRole role{ RuntimeRole::Launcher };
    std::string name;
    std::string executableName;
    std::unique_ptr<HostProcess> process;
};

void
appendLogLine(std::ostringstream& log, std::string_view line)
{
    log << line << '\n';
}

std::string_view
shortRoleName(RuntimeRole role)
{
    switch (role)
    {
    case RuntimeRole::Model:
        return "model";
    case RuntimeRole::Controller:
        return "controller";
    case RuntimeRole::View:
        return "view";
    case RuntimeRole::Launcher:
        return "launcher";
    case RuntimeRole::Broadcast:
        return "broadcast";
    }

    return "unknown";
}

std::filesystem::path
hostExecutable(const std::filesystem::path& hostDirectory, std::string_view hostName)
{
#ifdef _WIN32
    return hostDirectory / (std::string(hostName) + ".exe");
#else
    return hostDirectory / std::string(hostName);
#endif
}

int
tickCountForDuration(int durationMilliseconds, int tickMilliseconds)
{
    const auto normalizedDuration = std::max(durationMilliseconds, 1);
    const auto normalizedTick = std::max(tickMilliseconds, 1);
    return std::max(1, (normalizedDuration + normalizedTick - 1) / normalizedTick);
}

RuntimeEnvelope
lifecycle(RuntimeRole target, std::string name, std::uint64_t sequenceId, std::string_view sessionId)
{
    auto envelope = protocol::lifecycle(RuntimeRole::Launcher, target, std::move(name));
    envelope.sessionId = std::string(sessionId);
    envelope.sequenceId = sequenceId;
    return envelope;
}

RuntimeEnvelope
command(RuntimeRole source,
        RuntimeRole target,
        std::string name,
        std::string payload,
        std::uint64_t sequenceId,
        std::string_view sessionId)
{
    RuntimeEnvelope envelope;
    envelope.sessionId = std::string(sessionId);
    envelope.sequenceId = sequenceId;
    envelope.sourceRole = source;
    envelope.targetRole = target;
    envelope.kind = RuntimeMessageKind::Command;
    envelope.name = std::move(name);
    envelope.payload = std::move(payload);
    return envelope;
}

RuntimeEnvelope
heartbeat(RuntimeRole target, std::uint64_t sequenceId, std::string_view sessionId)
{
    RuntimeEnvelope envelope;
    envelope.sessionId = std::string(sessionId);
    envelope.sequenceId = sequenceId;
    envelope.sourceRole = RuntimeRole::Launcher;
    envelope.targetRole = target;
    envelope.kind = RuntimeMessageKind::Heartbeat;
    envelope.name = protocol::RuntimeHeartbeat;
    return envelope;
}

RuntimeHost
makeHost(RuntimeRole role, std::string name, std::string executableName)
{
    RuntimeHost host;
    host.role = role;
    host.name = std::move(name);
    host.executableName = std::move(executableName);
    return host;
}

bool
usesSceneFrame(std::string_view viewId)
{
    return viewId == runtime::RuntimeConfig::DefaultViewId ||
           viewId == "celestia.view3d.opengl";
}

std::string
viewExecutableName(std::string_view viewId)
{
    return usesSceneFrame(viewId) ? "celestia-view3d-host" : "celestia-view-host";
}

std::string
controllerTickPayload(int tick, const RuntimeSessionOptions& options)
{
    auto payload = "tick=" + std::to_string(tick);
    if (usesSceneFrame(options.viewId))
    {
        payload += ";view=" + options.viewId;
        payload += ";dt=" + std::to_string(std::max(options.tickMilliseconds, 1) / 1000.0);
    }

    return payload;
}

bool
startHost(RuntimeHost& host,
          const RuntimeSessionOptions& options,
          std::ostringstream& log)
{
    const auto executable = hostExecutable(options.runtimeHostDirectory, host.executableName);
    appendLogLine(log, host.name + " executable=" + host.executableName);
    if (!std::filesystem::exists(executable))
    {
        appendLogLine(log, host.name + " executable missing: " + executable.string());
        return false;
    }

    HostProcessOptions processOptions;
    processOptions.executable = executable;
    processOptions.workingDirectory = options.runtimeHostDirectory;
    processOptions.arguments = {
        "--stdio",
        "--protocol-version=1",
        "--view=" + options.viewId,
        "--serve",
        "--session=" + options.sessionId,
    };

    host.process = std::make_unique<HostProcess>(std::move(processOptions));
    if (!host.process->start())
    {
        appendLogLine(log, host.name + " failed to start");
        return false;
    }

    return true;
}

bool
send(RuntimeHost& host, const RuntimeEnvelope& envelope, std::ostringstream& log)
{
    if (host.process == nullptr || !host.process->send(envelope))
    {
        appendLogLine(log, host.name + " send failed: " + envelope.name);
        return false;
    }

    return true;
}

std::optional<RuntimeEnvelope>
receive(RuntimeHost& host, std::chrono::milliseconds timeout, std::ostringstream& log)
{
    if (host.process == nullptr)
    {
        appendLogLine(log, host.name + " process not started");
        return std::nullopt;
    }

    auto message = host.process->receive(timeout);
    if (!message.has_value())
    {
        appendLogLine(log, host.name + " receive timeout");
        return std::nullopt;
    }

    if (message->kind == RuntimeMessageKind::Error)
    {
        appendLogLine(log, host.name + " runtime.error name=" + message->name +
            " payload=" + message->payload);
        return std::nullopt;
    }

    return message;
}

std::optional<RuntimeEnvelope>
receiveAvailable(RuntimeHost& host)
{
    if (host.process == nullptr)
        return std::nullopt;

    return host.process->receive(std::chrono::milliseconds(0));
}

bool
routePendingViewInputs(RuntimeHost& view,
                       RuntimeHost& controller,
                       RuntimeHost& model,
                       RuntimeSessionResult& result,
                       std::chrono::milliseconds timeout,
                       std::ostringstream& log,
                       bool& shutdownRequested)
{
    constexpr int MaxInputsPerFrame = 64;

    for (int routed = 0; routed < MaxInputsPerFrame; ++routed)
    {
        const auto viewMessage = receiveAvailable(view);
        if (!viewMessage.has_value())
            return true;

        if (viewMessage->kind == RuntimeMessageKind::Error)
        {
            appendLogLine(log, "view runtime.error name=" + viewMessage->name +
                " payload=" + viewMessage->payload);
            return false;
        }

        if (viewMessage->kind != RuntimeMessageKind::Event ||
            viewMessage->name != protocol::ViewInputMessageName)
        {
            appendLogLine(log, "view returned unexpected " + viewMessage->name);
            return false;
        }

        if (!send(controller, *viewMessage, log))
            return false;

        const auto controllerMessage = receive(controller, timeout, log);
        if (!controllerMessage.has_value())
            return false;

        if (controllerMessage->targetRole == RuntimeRole::Broadcast &&
            controllerMessage->kind == RuntimeMessageKind::Lifecycle &&
            controllerMessage->name == protocol::RuntimeShutdown)
        {
            ++result.viewInputCount;
            shutdownRequested = true;
            appendLogLine(log, "view.input requested shutdown");
            return true;
        }

        if (controllerMessage->targetRole != RuntimeRole::Model ||
            controllerMessage->kind != RuntimeMessageKind::Command ||
            controllerMessage->name != "model.setViewInput")
        {
            appendLogLine(log, "controller routed unexpected " + controllerMessage->name +
                " to " + std::string(shortRoleName(controllerMessage->targetRole)));
            return false;
        }

        if (!send(model, *controllerMessage, log))
            return false;

        const auto modelMessage = receive(model, timeout, log);
        if (!modelMessage.has_value())
            return false;

        if (modelMessage->targetRole != RuntimeRole::View ||
            modelMessage->kind != RuntimeMessageKind::ViewFrame ||
            modelMessage->name != protocol::SceneFrameMessageName)
        {
            appendLogLine(log, "model routed unexpected " + modelMessage->name +
                " to " + std::string(shortRoleName(modelMessage->targetRole)));
            return false;
        }

        ++result.viewInputCount;
    }

    appendLogLine(log, "view.input route limit reached");
    return false;
}

bool
expectLifecycle(RuntimeHost& host,
                std::string_view expectedName,
                std::chrono::milliseconds timeout,
                std::ostringstream& log)
{
    const auto message = receive(host, timeout, log);
    if (!message.has_value())
        return false;

    if (message->kind != RuntimeMessageKind::Lifecycle || message->name != expectedName)
    {
        appendLogLine(log, host.name + " expected " + std::string(expectedName) +
            " but received " + message->name);
        return false;
    }

    return true;
}

bool
consumeStartup(RuntimeHost& host,
               std::chrono::milliseconds timeout,
               std::ostringstream& log)
{
    const auto message = receive(host, timeout, log);
    if (!message.has_value())
        return false;

    if (message->kind != RuntimeMessageKind::Lifecycle &&
        !(message->kind == RuntimeMessageKind::Event && message->name == "view.ready3d"))
    {
        appendLogLine(log, host.name + " expected startup lifecycle but received " + message->name);
        return false;
    }

    appendLogLine(log, host.name + " started");
    return true;
}

bool
expectHeartbeat(RuntimeHost& host,
                RuntimeSessionResult& result,
                std::uint64_t& sequenceId,
                const RuntimeSessionOptions& options,
                std::ostringstream& log)
{
    if (!send(host, heartbeat(host.role, sequenceId++, options.sessionId), log))
        return false;

    const auto message = receive(host,
                                 std::chrono::milliseconds(options.hostReadyTimeoutMilliseconds),
                                 log);
    if (!message.has_value())
        return false;

    if (message->kind != RuntimeMessageKind::Heartbeat ||
        message->name != protocol::RuntimeHeartbeat)
    {
        appendLogLine(log, host.name + " expected heartbeat but received " + message->name);
        return false;
    }

    ++result.heartbeatCount;
    return true;
}

bool
shutdownHost(RuntimeHost& host,
             RuntimeSessionResult& result,
             std::uint64_t& sequenceId,
             const RuntimeSessionOptions& options,
             std::ostringstream& log)
{
    if (host.process == nullptr)
        return false;

    if (!send(host, lifecycle(host.role, protocol::RuntimeShutdown, sequenceId++, options.sessionId), log))
        return false;

    if (options.shutdownTimeoutMilliseconds <= 0)
    {
        appendLogLine(log, host.name + " shutdown wait timeout");
        if (host.process->terminate())
            result.terminatedHost = true;
        return false;
    }

    const auto stopped = expectLifecycle(host, protocol::RuntimeStopped,
                                         std::chrono::milliseconds(options.shutdownTimeoutMilliseconds),
                                         log);

    const auto exitCode = host.process->wait(
        std::chrono::milliseconds(options.shutdownTimeoutMilliseconds));
    if (!exitCode.has_value())
    {
        appendLogLine(log, host.name + " shutdown wait timeout");
        if (host.process->terminate())
            result.terminatedHost = true;
    }

    if (host.role == RuntimeRole::Controller)
    {
        result.controllerStopped = stopped;
        result.controllerExitCode = exitCode.value_or(-1);
    }
    else if (host.role == RuntimeRole::Model)
    {
        result.modelStopped = stopped;
        result.modelExitCode = exitCode.value_or(-1);
    }
    else if (host.role == RuntimeRole::View)
    {
        result.viewStopped = stopped;
        result.viewExitCode = exitCode.value_or(-1);
    }

    return stopped && exitCode.has_value() && *exitCode == 0;
}

} // end unnamed namespace

RuntimeSession::RuntimeSession(RuntimeSessionOptions options)
    : options_(std::move(options))
{
}

RuntimeSessionResult
RuntimeSession::run()
{
    RuntimeSessionResult result;
    std::ostringstream log;
    std::uint64_t sequenceId = 1;

    appendLogLine(log, "sessionId=" + options_.sessionId);

    auto controller = makeHost(RuntimeRole::Controller, "controller", "celestia-controller-host");
    auto model = makeHost(RuntimeRole::Model, "model", "celestia-model-host");
    auto view = makeHost(RuntimeRole::View, "view", viewExecutableName(options_.viewId));
    const auto sceneFrameMode = usesSceneFrame(options_.viewId);

    if (!startHost(controller, options_, log) ||
        !startHost(model, options_, log) ||
        !startHost(view, options_, log))
    {
        result.log = log.str();
        return result;
    }

    const auto readyTimeout = std::chrono::milliseconds(options_.hostReadyTimeoutMilliseconds);

    if (send(controller, lifecycle(RuntimeRole::Controller, protocol::RuntimeHello, sequenceId++, options_.sessionId), log) &&
        expectLifecycle(controller, protocol::RuntimeReady, readyTimeout, log))
    {
        result.controllerReady = true;
        appendLogLine(log, "controller ready");
    }

    if (send(model, lifecycle(RuntimeRole::Model, protocol::RuntimeHello, sequenceId++, options_.sessionId), log) &&
        expectLifecycle(model, protocol::RuntimeReady, readyTimeout, log))
    {
        result.modelReady = true;
        appendLogLine(log, "model ready");
    }

    if (send(view, lifecycle(RuntimeRole::View, protocol::RuntimeHello, sequenceId++, options_.sessionId), log) &&
        expectLifecycle(view, protocol::RuntimeReady, readyTimeout, log))
    {
        result.viewReady = true;
        appendLogLine(log, "view ready");
    }

    if (!result.controllerReady || !result.modelReady || !result.viewReady)
    {
        result.log = log.str();
        return result;
    }

    const auto startTimeout = std::chrono::milliseconds(options_.hostReadyTimeoutMilliseconds);
    if (!send(controller, lifecycle(RuntimeRole::Controller, protocol::RuntimeStart, sequenceId++, options_.sessionId), log) ||
        !consumeStartup(controller, startTimeout, log) ||
        !send(model, lifecycle(RuntimeRole::Model, protocol::RuntimeStart, sequenceId++, options_.sessionId), log) ||
        !consumeStartup(model, startTimeout, log) ||
        !send(view, lifecycle(RuntimeRole::View, protocol::RuntimeStart, sequenceId++, options_.sessionId), log) ||
        !consumeStartup(view, startTimeout, log))
    {
        result.log = log.str();
        return result;
    }

    const auto tickCount = tickCountForDuration(options_.durationMilliseconds, options_.tickMilliseconds);
    const auto tickTimeout = std::chrono::milliseconds(std::max(options_.tickMilliseconds * 10, 100));
    int renderedFrameCount = 0;
    bool shutdownRequested = false;
    for (int tick = 0; tick < tickCount; ++tick)
    {
        const auto tickMessage = command(RuntimeRole::Launcher,
                                         RuntimeRole::Controller,
                                         options_.controllerTickCommandName,
                                         controllerTickPayload(tick, options_),
                                         sequenceId++,
                                         options_.sessionId);
        if (!send(controller, tickMessage, log))
            break;

        const auto controllerMessage = receive(controller, tickTimeout, log);
        if (!controllerMessage.has_value())
            break;

        if (controllerMessage->targetRole != RuntimeRole::Model ||
            controllerMessage->kind != RuntimeMessageKind::Command)
        {
            appendLogLine(log, "controller routed unexpected " + controllerMessage->name +
                " to " + std::string(shortRoleName(controllerMessage->targetRole)));
            break;
        }

        if (!send(model, *controllerMessage, log))
            break;

        const auto modelMessage = receive(model, tickTimeout, log);
        if (!modelMessage.has_value())
            break;

        const auto expectedFrameName = sceneFrameMode
            ? std::string(protocol::SceneFrameMessageName)
            : std::string("view.frame");

        if (modelMessage->targetRole != RuntimeRole::View ||
            modelMessage->kind != RuntimeMessageKind::ViewFrame ||
            modelMessage->name != expectedFrameName)
        {
            appendLogLine(log, "model routed unexpected " + modelMessage->name +
                " to " + std::string(shortRoleName(modelMessage->targetRole)));
            break;
        }

        if (!send(view, *modelMessage, log))
            break;

        if (sceneFrameMode)
        {
            const auto viewMessage = receive(view, tickTimeout, log);
            if (!viewMessage.has_value())
                break;

            if (viewMessage->kind != RuntimeMessageKind::Event ||
                viewMessage->name != "view.frameRendered")
            {
                appendLogLine(log, "view returned unexpected " + viewMessage->name);
                break;
            }

            ++renderedFrameCount;
            if (!routePendingViewInputs(view, controller, model, result, tickTimeout,
                                        log, shutdownRequested))
            {
                break;
            }
            if (shutdownRequested)
                break;
        }

        ++result.tickCount;
        ++result.viewFrameCount;

        if (options_.heartbeatEveryTicks > 0 &&
            (tick + 1) % options_.heartbeatEveryTicks == 0)
        {
            if (!expectHeartbeat(controller, result, sequenceId, options_, log) ||
                !expectHeartbeat(model, result, sequenceId, options_, log) ||
                !expectHeartbeat(view, result, sequenceId, options_, log))
            {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(options_.tickMilliseconds, 1)));
    }

    appendLogLine(log, "tick count=" + std::to_string(result.tickCount));
    if (sceneFrameMode)
    {
        appendLogLine(log, "scene.frame count=" + std::to_string(result.viewFrameCount));
        appendLogLine(log, "view.frameRendered count=" + std::to_string(renderedFrameCount));
        appendLogLine(log, "view.input routed count=" + std::to_string(result.viewInputCount));
    }
    else
    {
        appendLogLine(log, "view.frame count=" + std::to_string(result.viewFrameCount));
    }
    appendLogLine(log, "heartbeat count=" + std::to_string(result.heartbeatCount));

    const auto controllerStopped = shutdownHost(controller, result, sequenceId, options_, log);
    const auto modelStopped = shutdownHost(model, result, sequenceId, options_, log);
    const auto viewStopped = shutdownHost(view, result, sequenceId, options_, log);

    if (controllerStopped && modelStopped && viewStopped)
        appendLogLine(log, "all hosts stopped");

    result.success = result.controllerReady && result.modelReady && result.viewReady &&
                     result.controllerStopped && result.modelStopped && result.viewStopped &&
                     result.controllerExitCode == 0 && result.modelExitCode == 0 &&
                     result.viewExitCode == 0 && result.tickCount == tickCount &&
                     result.viewFrameCount == tickCount &&
                     (!sceneFrameMode || renderedFrameCount == tickCount);
    result.log = log.str();
    return result;
}

} // namespace celestia::runtime::process
