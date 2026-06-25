// runtimeconfig.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimeconfig.h"

#include <string>
#include <utility>

namespace celestia::runtime
{

std::string_view
runtimeModeName(RuntimeMode mode)
{
    switch (mode)
    {
    case RuntimeMode::InProcessDirect:
        return "in-process";
    case RuntimeMode::InProcessChannel:
        return "in-process-channel";
    case RuntimeMode::MultiProcess:
        return "multi-process";
    }

    return "in-process";
}

std::optional<RuntimeMode>
runtimeModeFromString(std::string_view mode)
{
    if (mode == "in-process" || mode == "in_process_direct")
        return RuntimeMode::InProcessDirect;
    if (mode == "in-process-channel" || mode == "in_process_channel")
        return RuntimeMode::InProcessChannel;
    if (mode == "multi-process" || mode == "multi_process")
        return RuntimeMode::MultiProcess;
    return std::nullopt;
}

RuntimeConfig::RuntimeConfig() :
    m_selectedViewId(DefaultViewId),
    m_hostTransport("stdio-pipe"),
    m_runtimeMode(DefaultRuntimeMode),
    m_runOnce(false),
    m_serve(false),
    m_listViews(false),
    m_durationMilliseconds(500),
    m_switchViewAfterMilliseconds(0)
{
}

const std::string&
RuntimeConfig::selectedViewId() const
{
    return m_selectedViewId;
}

void
RuntimeConfig::setSelectedViewId(std::string viewId)
{
    m_selectedViewId = std::move(viewId);
}

RuntimeMode
RuntimeConfig::runtimeMode() const
{
    return m_runtimeMode;
}

void
RuntimeConfig::setRuntimeMode(RuntimeMode mode)
{
    m_runtimeMode = mode;
}

bool
RuntimeConfig::runOnce() const
{
    return m_runOnce;
}

void
RuntimeConfig::setRunOnce(bool runOnce)
{
    m_runOnce = runOnce;
}

bool
RuntimeConfig::serve() const
{
    return m_serve;
}

void
RuntimeConfig::setServe(bool serve)
{
    m_serve = serve;
}

int
RuntimeConfig::durationMilliseconds() const
{
    return m_durationMilliseconds;
}

void
RuntimeConfig::setDurationMilliseconds(int durationMilliseconds)
{
    m_durationMilliseconds = durationMilliseconds;
}

std::string_view
RuntimeConfig::hostTransport() const
{
    return m_hostTransport;
}

void
RuntimeConfig::setHostTransport(std::string hostTransport)
{
    m_hostTransport = std::move(hostTransport);
}

bool
RuntimeConfig::listViews() const
{
    return m_listViews;
}

void
RuntimeConfig::setListViews(bool listViews)
{
    m_listViews = listViews;
}

const std::string&
RuntimeConfig::pluginDirectory() const
{
    return m_pluginDirectory;
}

void
RuntimeConfig::setPluginDirectory(std::string pluginDirectory)
{
    m_pluginDirectory = std::move(pluginDirectory);
}

int
RuntimeConfig::switchViewAfterMilliseconds() const
{
    return m_switchViewAfterMilliseconds;
}

void
RuntimeConfig::setSwitchViewAfterMilliseconds(int switchViewAfterMilliseconds)
{
    m_switchViewAfterMilliseconds = switchViewAfterMilliseconds;
}

const std::string&
RuntimeConfig::switchViewId() const
{
    return m_switchViewId;
}

void
RuntimeConfig::setSwitchViewId(std::string switchViewId)
{
    m_switchViewId = std::move(switchViewId);
}

const std::string&
RuntimeConfig::runtimeConfigPath() const
{
    return m_runtimeConfigPath;
}

void
RuntimeConfig::setRuntimeConfigPath(std::string runtimeConfigPath)
{
    m_runtimeConfigPath = std::move(runtimeConfigPath);
}

namespace
{

std::optional<int>
parseInt(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoi(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

} // end unnamed namespace

bool
applyRuntimeConfigArgument(RuntimeConfig& config, std::string_view argument)
{
    constexpr std::string_view viewOption{ "--view=" };
    constexpr std::string_view modeOption{ "--mvc-mode=" };
    constexpr std::string_view durationOption{ "--duration-ms=" };
    constexpr std::string_view transportOption{ "--host-transport=" };
    constexpr std::string_view pluginDirOption{ "--plugin-dir=" };
    constexpr std::string_view switchAfterOption{ "--switch-view-after-ms=" };
    constexpr std::string_view switchViewOption{ "--switch-view=" };
    constexpr std::string_view runtimeConfigOption{ "--runtime-config=" };

    if (argument.compare(0, viewOption.size(), viewOption) == 0)
    {
        config.setSelectedViewId(std::string(argument.substr(viewOption.size())));
        return true;
    }

    if (argument.compare(0, modeOption.size(), modeOption) == 0)
    {
        const auto mode = runtimeModeFromString(argument.substr(modeOption.size()));
        if (!mode.has_value())
            return false;

        config.setRuntimeMode(*mode);
        return true;
    }

    if (argument == "--once")
    {
        config.setRunOnce(true);
        return true;
    }

    if (argument == "--serve")
    {
        config.setServe(true);
        return true;
    }

    if (argument == "--list-views")
    {
        config.setListViews(true);
        return true;
    }

    if (argument.compare(0, durationOption.size(), durationOption) == 0)
    {
        const auto duration = parseInt(argument.substr(durationOption.size()));
        if (!duration.has_value() || *duration <= 0)
            return false;

        config.setDurationMilliseconds(*duration);
        return true;
    }

    if (argument.compare(0, pluginDirOption.size(), pluginDirOption) == 0)
    {
        config.setPluginDirectory(std::string(argument.substr(pluginDirOption.size())));
        return true;
    }

    if (argument.compare(0, switchAfterOption.size(), switchAfterOption) == 0)
    {
        const auto value = parseInt(argument.substr(switchAfterOption.size()));
        if (!value.has_value() || *value <= 0)
            return false;

        config.setSwitchViewAfterMilliseconds(*value);
        return true;
    }

    if (argument.compare(0, switchViewOption.size(), switchViewOption) == 0)
    {
        config.setSwitchViewId(std::string(argument.substr(switchViewOption.size())));
        return true;
    }

    if (argument.compare(0, transportOption.size(), transportOption) == 0)
    {
        const auto transport = argument.substr(transportOption.size());
        if (transport == "stdio" || transport == "stdio-pipe")
        {
            config.setHostTransport("stdio-pipe");
            return true;
        }

        if (transport == "stdio-files")
        {
            config.setHostTransport("stdio-files");
            return true;
        }

        if (transport == "local-socket" || transport == "local_socket")
        {
            config.setHostTransport("local-socket");
            return true;
        }

        return false;
    }

    if (argument.compare(0, runtimeConfigOption.size(), runtimeConfigOption) == 0)
    {
        config.setRuntimeConfigPath(std::string(argument.substr(runtimeConfigOption.size())));
        return true;
    }

    return false;
}

} // namespace celestia::runtime
