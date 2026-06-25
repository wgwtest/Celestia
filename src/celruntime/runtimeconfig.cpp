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
        return "in_process_direct";
    case RuntimeMode::InProcessChannel:
        return "in_process_channel";
    }

    return "in_process_direct";
}

std::optional<RuntimeMode>
runtimeModeFromString(std::string_view mode)
{
    if (mode == "in_process_direct")
        return RuntimeMode::InProcessDirect;
    if (mode == "in_process_channel")
        return RuntimeMode::InProcessChannel;
    return std::nullopt;
}

RuntimeConfig::RuntimeConfig() :
    m_selectedViewId(DefaultViewId),
    m_runtimeMode(DefaultRuntimeMode)
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

} // namespace celestia::runtime
