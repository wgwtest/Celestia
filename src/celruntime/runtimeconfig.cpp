// runtimeconfig.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimeconfig.h"

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
    m_runOnce(false)
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
applyRuntimeConfigArgument(RuntimeConfig& config, std::string_view argument)
{
    constexpr std::string_view viewOption{ "--view=" };
    constexpr std::string_view modeOption{ "--mvc-mode=" };

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

    return false;
}

} // namespace celestia::runtime
