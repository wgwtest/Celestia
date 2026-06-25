// runtimeconfig.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace celestia::runtime
{

enum class RuntimeMode
{
    InProcessDirect,
    InProcessChannel,
};

std::string_view runtimeModeName(RuntimeMode);
std::optional<RuntimeMode> runtimeModeFromString(std::string_view);

class RuntimeConfig
{
public:
    static constexpr std::string_view DefaultViewId{ "celestia.view3d.opengl" };
    static constexpr RuntimeMode DefaultRuntimeMode{ RuntimeMode::InProcessDirect };

    RuntimeConfig();

    const std::string& selectedViewId() const;
    void setSelectedViewId(std::string);

    RuntimeMode runtimeMode() const;
    void setRuntimeMode(RuntimeMode);

private:
    std::string m_selectedViewId;
    RuntimeMode m_runtimeMode;
};

} // namespace celestia::runtime
