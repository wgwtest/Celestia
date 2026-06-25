// processsupervisor.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "processsupervisor.h"

#include "runtimesession.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/transport/framedmessage.h>

namespace celestia::runtime::process
{
namespace
{

using protocol::RuntimeEnvelope;
using protocol::RuntimeMessageKind;
using protocol::RuntimeRole;

struct HostRunResult
{
    int exitCode{ -1 };
    std::vector<RuntimeEnvelope> messages;
    std::string output;
};

bool
writeText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path, std::ios::binary);
    if (!output.good())
        return false;

    output << text;
    return true;
}

std::optional<std::string>
readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        return std::nullopt;

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string
quotePath(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
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

std::string
frameMessages(const std::vector<RuntimeEnvelope>& messages)
{
    std::string output;
    for (const auto& message : messages)
        output += transport::encodeFrame(message);
    return output;
}

std::vector<RuntimeEnvelope>
parseFrames(std::string_view output)
{
    transport::FramedMessageReader reader;
    reader.append(std::string(output));

    std::vector<RuntimeEnvelope> messages;
    for (;;)
    {
        const auto received = reader.receive();
        if (received.status == transport::ReceiveStatus::Message && received.message.has_value())
        {
            messages.push_back(*received.message);
            continue;
        }

        break;
    }

    return messages;
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

HostRunResult
runHost(const ProcessSupervisorOptions& options,
        std::string_view hostName,
        const std::vector<RuntimeEnvelope>& messages)
{
    HostRunResult result;

    const auto executable = hostExecutable(options.runtimeHostDirectory, hostName);
    if (!std::filesystem::exists(executable))
    {
        result.output = "missing host executable: " + executable.string();
        return result;
    }

    const auto baseName = "celestia-step6-" + options.sessionId + "-" + std::string(hostName);
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto inputPath = tempDir / (baseName + ".in");
    const auto outputPath = tempDir / (baseName + ".out");
    const auto scriptPath = tempDir / (baseName + ".cmd");

    if (!writeText(inputPath, frameMessages(messages)))
    {
        result.output = "failed to write host input";
        return result;
    }

    std::filesystem::remove(outputPath);

#ifdef _WIN32
    if (!writeText(scriptPath,
                   "@echo off\n" +
                       quotePath(executable) +
                       " --stdio --protocol-version=1 --view=" + options.viewId +
                       " --serve --session=" + options.sessionId +
                       " < " + quotePath(inputPath) +
                       " > " + quotePath(outputPath) + "\n" +
                       "exit /b %ERRORLEVEL%\n"))
    {
        result.output = "failed to write host command script";
        return result;
    }

    const auto systemCommand = "cmd.exe /d /c call " + quotePath(scriptPath);
#else
    const auto systemCommand =
        quotePath(executable) +
        " --stdio --protocol-version=1 --view=" + options.viewId +
        " --serve --session=" + options.sessionId +
        " < " + quotePath(inputPath) +
        " > " + quotePath(outputPath);
#endif

    result.exitCode = std::system(systemCommand.c_str());
    const auto output = readText(outputPath);
    if (!output.has_value())
    {
        result.output = "host produced no output";
        return result;
    }

    result.output = *output;
    result.messages = parseFrames(*output);
    return result;
}

bool
hasMessage(const std::vector<RuntimeEnvelope>& messages,
           RuntimeRole source,
           RuntimeMessageKind kind,
           std::string_view name)
{
    for (const auto& message : messages)
    {
        if (message.sourceRole == source && message.kind == kind && message.name == name)
            return true;
    }

    return false;
}

std::vector<RuntimeEnvelope>
messagesNamed(const std::vector<RuntimeEnvelope>& messages, std::string_view name)
{
    std::vector<RuntimeEnvelope> selected;
    for (const auto& message : messages)
    {
        if (message.name == name)
            selected.push_back(message);
    }

    return selected;
}

void
appendLogLine(std::ostringstream& log, std::string_view line)
{
    log << line << '\n';
}

int
tickCountForDuration(int durationMilliseconds)
{
    if (durationMilliseconds <= 0)
        return 1;

    const auto tickCount = (durationMilliseconds + 249) / 250;
    return tickCount < 1 ? 1 : tickCount;
}

} // end unnamed namespace

ProcessSupervisor::ProcessSupervisor(ProcessSupervisorOptions options)
    : options_(std::move(options))
{
}

ProcessSupervisorResult
ProcessSupervisor::runServeSmoke() const
{
    ProcessSupervisorResult result;
    std::ostringstream log;

    if (options_.hostTransport != "stdio" && options_.hostTransport != "stdio-files")
    {
        appendLogLine(log, "unsupported host transport: " + options_.hostTransport);
        result.log = log.str();
        return result;
    }

    std::uint64_t sequenceId = 1;
    const auto tickCount = tickCountForDuration(options_.durationMilliseconds);

    std::vector<RuntimeEnvelope> controllerMessages = {
        lifecycle(RuntimeRole::Controller, protocol::RuntimeHello, sequenceId++, options_.sessionId),
        lifecycle(RuntimeRole::Controller, protocol::RuntimeStart, sequenceId++, options_.sessionId),
    };
    for (int tick = 0; tick < tickCount; ++tick)
    {
        controllerMessages.push_back(command(RuntimeRole::Launcher,
                                             RuntimeRole::Controller,
                                             "controller.tick",
                                             "tick=" + std::to_string(tick),
                                             sequenceId++,
                                             options_.sessionId));
    }
    controllerMessages.push_back(
        lifecycle(RuntimeRole::Controller, protocol::RuntimeShutdown, sequenceId++, options_.sessionId));

    const auto controller = runHost(options_, "celestia-controller-host", controllerMessages);
    result.controllerExitCode = controller.exitCode;
    result.controllerReady = hasMessage(controller.messages, RuntimeRole::Controller,
                                        RuntimeMessageKind::Lifecycle, protocol::RuntimeReady);
    if (result.controllerReady)
        appendLogLine(log, "controller ready");

    std::vector<RuntimeEnvelope> modelMessages = {
        lifecycle(RuntimeRole::Model, protocol::RuntimeHello, sequenceId++, options_.sessionId),
        lifecycle(RuntimeRole::Model, protocol::RuntimeStart, sequenceId++, options_.sessionId),
    };
    for (int tick = 0; tick < tickCount; ++tick)
    {
        modelMessages.push_back(command(RuntimeRole::Controller,
                                        RuntimeRole::Model,
                                        "model.step",
                                        "dt=0.25",
                                        sequenceId++,
                                        options_.sessionId));
    }
    for (auto message : controller.messages)
    {
        if (message.targetRole == RuntimeRole::Model && message.kind == RuntimeMessageKind::Command)
            modelMessages.push_back(std::move(message));
    }
    modelMessages.push_back(
        lifecycle(RuntimeRole::Model, protocol::RuntimeShutdown, sequenceId++, options_.sessionId));

    const auto model = runHost(options_, "celestia-model-host", modelMessages);
    result.modelExitCode = model.exitCode;
    result.modelReady = hasMessage(model.messages, RuntimeRole::Model,
                                   RuntimeMessageKind::Lifecycle, protocol::RuntimeReady);
    if (result.modelReady)
        appendLogLine(log, "model ready");

    auto viewFrames = messagesNamed(model.messages, "view.frame");
    result.viewFrameCount = static_cast<int>(viewFrames.size());
    if (result.viewFrameCount > 0)
        appendLogLine(log, "view.frame count=" + std::to_string(result.viewFrameCount));

    std::vector<RuntimeEnvelope> viewMessages = {
        lifecycle(RuntimeRole::View, protocol::RuntimeHello, sequenceId++, options_.sessionId),
        lifecycle(RuntimeRole::View, protocol::RuntimeStart, sequenceId++, options_.sessionId),
    };
    for (auto frame : viewFrames)
    {
        frame.targetRole = RuntimeRole::View;
        viewMessages.push_back(std::move(frame));
    }
    viewMessages.push_back(
        lifecycle(RuntimeRole::View, protocol::RuntimeShutdown, sequenceId++, options_.sessionId));

    const auto view = runHost(options_, "celestia-view-host", viewMessages);
    result.viewExitCode = view.exitCode;
    result.viewReady = hasMessage(view.messages, RuntimeRole::View,
                                  RuntimeMessageKind::Lifecycle, protocol::RuntimeReady);
    if (result.viewReady)
        appendLogLine(log, "view ready");

    const auto allStopped =
        controller.exitCode == 0 && model.exitCode == 0 && view.exitCode == 0 &&
        hasMessage(controller.messages, RuntimeRole::Controller,
                   RuntimeMessageKind::Lifecycle, protocol::RuntimeStopped) &&
        hasMessage(model.messages, RuntimeRole::Model,
                   RuntimeMessageKind::Lifecycle, protocol::RuntimeStopped) &&
        hasMessage(view.messages, RuntimeRole::View,
                   RuntimeMessageKind::Lifecycle, protocol::RuntimeStopped);
    if (allStopped)
    {
        appendLogLine(log, "all hosts stopped");
        result.controllerStopped = true;
        result.modelStopped = true;
        result.viewStopped = true;
    }

    result.success = result.modelReady && result.controllerReady && result.viewReady &&
                     result.viewFrameCount >= 1 && allStopped;
    result.log = log.str();
    return result;
}

ProcessSupervisorResult
ProcessSupervisor::runRuntime() const
{
    ProcessSupervisorResult result;
    std::ostringstream log;

    if (options_.hostTransport != "stdio-pipe" && options_.hostTransport != "stdio")
    {
        appendLogLine(log, "unsupported host transport: " + options_.hostTransport);
        result.log = log.str();
        return result;
    }

    RuntimeSessionOptions sessionOptions;
    sessionOptions.runtimeHostDirectory = options_.runtimeHostDirectory;
    sessionOptions.viewId = options_.viewId;
    sessionOptions.sessionId = options_.sessionId;
    sessionOptions.durationMilliseconds = options_.durationMilliseconds;

    RuntimeSession session(sessionOptions);
    const auto sessionResult = session.run();

    result.success = sessionResult.success;
    result.modelReady = sessionResult.modelReady;
    result.controllerReady = sessionResult.controllerReady;
    result.viewReady = sessionResult.viewReady;
    result.modelStopped = sessionResult.modelStopped;
    result.controllerStopped = sessionResult.controllerStopped;
    result.viewStopped = sessionResult.viewStopped;
    result.modelExitCode = sessionResult.modelExitCode;
    result.controllerExitCode = sessionResult.controllerExitCode;
    result.viewExitCode = sessionResult.viewExitCode;
    result.tickCount = sessionResult.tickCount;
    result.viewFrameCount = sessionResult.viewFrameCount;
    result.heartbeatCount = sessionResult.heartbeatCount;
    result.terminatedHost = sessionResult.terminatedHost;
    result.log = sessionResult.log;
    return result;
}

} // namespace celestia::runtime::process
