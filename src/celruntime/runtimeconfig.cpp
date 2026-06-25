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
    m_runtimeMode(DefaultRuntimeMode),
    m_runOnce(false),
    m_serve(false),
    m_durationMilliseconds(500)
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
    return "stdio";
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

    if (argument.compare(0, durationOption.size(), durationOption) == 0)
    {
        const auto duration = parseInt(argument.substr(durationOption.size()));
        if (!duration.has_value() || *duration <= 0)
            return false;

        config.setDurationMilliseconds(*duration);
        return true;
    }

    if (argument.compare(0, transportOption.size(), transportOption) == 0)
    {
        return argument.substr(transportOption.size()) == "stdio";
    }

    return false;
}

} // namespace celestia::runtime
