// runtimeassemblyrunner.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "runtimeassemblyrunner.h"

#include <fstream>
#include <sstream>
#include <utility>

#include <celruntime/viewplugin/viewpluginregistry.h>

namespace celestia::runtime::assembly
{
namespace
{

void
writeTrace(const RuntimeAssemblyConfig& config,
           const process::RuntimeSessionResult& result)
{
    if (config.supervisor.traceFile.empty())
        return;

    std::ofstream output(config.supervisor.traceFile, std::ios::binary);
    if (!output.good())
        return;

    output << "sessionId=" << config.session.id << '\n';
    output << "transport=" << config.transport.controlKind << '\n';
    output << "view=" << config.view.id << '\n';
    output << "success=" << (result.success ? "true" : "false") << '\n';
    output << "controller exitCode=" << result.controllerExitCode << '\n';
    output << "model exitCode=" << result.modelExitCode << '\n';
    output << "view exitCode=" << result.viewExitCode << '\n';
}

} // end unnamed namespace

RuntimeAssemblyRunner::RuntimeAssemblyRunner(RuntimeAssemblyConfig config)
    : config_(std::move(config))
{
}

process::RuntimeSessionResult
RuntimeAssemblyRunner::run() const
{
    auto registry = viewplugin::builtinViewPluginRegistry();
    std::string validationError;
    if (!config_.view.pluginDirectory.empty() &&
        !registry.discover(config_.view.pluginDirectory, &validationError))
    {
        process::RuntimeSessionResult result;
        result.log = "assembly validation failed: " + validationError + '\n';
        return result;
    }

    if (!config_.validate(registry, &validationError))
    {
        process::RuntimeSessionResult result;
        result.log = "assembly validation failed: " + validationError + '\n';
        return result;
    }

    process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = config_.runtimeHostDirectory;
    options.viewId = config_.view.id;
    options.sessionId = config_.session.id;
    options.durationMilliseconds = config_.session.durationMilliseconds;
    options.tickMilliseconds = config_.session.tickMilliseconds;
    options.hostReadyTimeoutMilliseconds = config_.session.readyTimeoutMilliseconds;
    options.shutdownTimeoutMilliseconds = config_.session.shutdownTimeoutMilliseconds;
    options.hostTransport = config_.transport.controlKind;
    options.switchViewAfterMilliseconds = config_.view.switchAfterMilliseconds;
    options.switchViewId = config_.view.switchViewId;

    process::RuntimeSession session(options);
    auto result = session.run();

    std::ostringstream log;
    log << "assembly view=" << config_.view.id << '\n';
    log << "transport=" << config_.transport.controlKind << '\n';
    log << result.log;
    result.log = log.str();

    writeTrace(config_, result);
    return result;
}

} // namespace celestia::runtime::assembly
