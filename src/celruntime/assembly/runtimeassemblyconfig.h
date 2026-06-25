// runtimeassemblyconfig.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <celruntime/runtimeconfig.h>
#include <celruntime/viewplugin/viewpluginregistry.h>

namespace celestia::runtime::assembly
{

struct RuntimeAssemblySessionConfig
{
    std::string id{ "default" };
    int durationMilliseconds{ 500 };
    int tickMilliseconds{ 125 };
    int heartbeatMilliseconds{ 1000 };
    int shutdownTimeoutMilliseconds{ 3000 };
    int readyTimeoutMilliseconds{ 3000 };
};

struct RuntimeAssemblyResourceConfig
{
    std::filesystem::path contentRoot;
};

struct RuntimeAssemblyTransportConfig
{
    std::string controlKind{ "stdio-pipe" };
    std::string dataKind{ "framed-envelope" };
};

struct RuntimeAssemblyHostConfig
{
    std::string hostName;
};

struct RuntimeAssemblyViewConfig
{
    std::string id{ std::string(RuntimeConfig::DefaultViewId) };
    std::string pluginDirectory;
    std::string hostName;
    int switchAfterMilliseconds{ 0 };
    std::string switchViewId;
};

struct RuntimeAssemblySupervisorConfig
{
    std::string restartPolicy{ "never" };
    int maxRestarts{ 0 };
    std::filesystem::path traceFile;
};

struct RuntimeAssemblyConfig
{
    RuntimeAssemblySessionConfig session;
    RuntimeAssemblyResourceConfig resources;
    RuntimeAssemblyTransportConfig transport;
    RuntimeAssemblyHostConfig model{ "celestia-model-host" };
    RuntimeAssemblyHostConfig controller{ "celestia-controller-host" };
    RuntimeAssemblyViewConfig view;
    RuntimeAssemblySupervisorConfig supervisor;
    std::filesystem::path runtimeHostDirectory;

    static RuntimeAssemblyConfig fromRuntimeConfig(const RuntimeConfig&,
                                                   std::filesystem::path runtimeHostDirectory,
                                                   std::filesystem::path contentRoot,
                                                   std::string sessionId);

    bool validate(const viewplugin::ViewPluginRegistry&, std::string* error = nullptr) const;
};

std::optional<RuntimeAssemblyConfig> loadRuntimeAssemblyConfig(
    const std::filesystem::path& path,
    std::filesystem::path runtimeHostDirectory,
    std::filesystem::path defaultContentRoot,
    std::string* error = nullptr);

} // namespace celestia::runtime::assembly
