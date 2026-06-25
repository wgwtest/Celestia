// processsupervisor.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "processsupervisor.h"

#include <sstream>
#include <string_view>
#include <utility>

#include <celruntime/assembly/runtimeassemblyrunner.h>
#include <celruntime/runtimeconfig.h>

namespace celestia::runtime::process
{
namespace
{

void
appendLogLine(std::ostringstream& log, std::string_view line)
{
    log << line << '\n';
}

} // end unnamed namespace

ProcessSupervisor::ProcessSupervisor(ProcessSupervisorOptions options)
    : options_(std::move(options))
{
}

ProcessSupervisorResult
ProcessSupervisor::runRuntime() const
{
    ProcessSupervisorResult result;
    std::ostringstream log;

    if (options_.hostTransport != "stdio-pipe" &&
        options_.hostTransport != "stdio" &&
        options_.hostTransport != "local-socket")
    {
        appendLogLine(log, "unsupported host transport: " + options_.hostTransport);
        result.log = log.str();
        return result;
    }

    RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(options_.viewId);
    runtimeConfig.setHostTransport(options_.hostTransport == "stdio"
        ? "stdio-pipe"
        : options_.hostTransport);
    runtimeConfig.setDurationMilliseconds(options_.durationMilliseconds);
    if (options_.switchViewAfterMilliseconds > 0)
        runtimeConfig.setSwitchViewAfterMilliseconds(options_.switchViewAfterMilliseconds);
    if (!options_.switchViewId.empty())
        runtimeConfig.setSwitchViewId(options_.switchViewId);

    auto assemblyConfig = assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        options_.runtimeHostDirectory,
        options_.runtimeHostDirectory,
        options_.sessionId);
    assembly::RuntimeAssemblyRunner runner(std::move(assemblyConfig));
    const auto sessionResult = runner.run();

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
