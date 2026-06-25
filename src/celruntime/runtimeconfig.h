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
    MultiProcess,
};

std::string_view runtimeModeName(RuntimeMode);
std::optional<RuntimeMode> runtimeModeFromString(std::string_view);

class RuntimeConfig
{
public:
    static constexpr std::string_view DefaultViewId{ "celestia.view3d.opengl" };
    static constexpr std::string_view Debug2DViewId{ "celestia.view2d.debug" };
    static constexpr RuntimeMode DefaultRuntimeMode{ RuntimeMode::InProcessDirect };

    RuntimeConfig();

    const std::string& selectedViewId() const;
    void setSelectedViewId(std::string);

    RuntimeMode runtimeMode() const;
    void setRuntimeMode(RuntimeMode);

    bool runOnce() const;
    void setRunOnce(bool);

    bool serve() const;
    void setServe(bool);

    int durationMilliseconds() const;
    void setDurationMilliseconds(int);

    std::string_view hostTransport() const;
    void setHostTransport(std::string);

    bool listViews() const;
    void setListViews(bool);

    const std::string& pluginDirectory() const;
    void setPluginDirectory(std::string);

    int switchViewAfterMilliseconds() const;
    void setSwitchViewAfterMilliseconds(int);

    const std::string& switchViewId() const;
    void setSwitchViewId(std::string);

    const std::string& runtimeConfigPath() const;
    void setRuntimeConfigPath(std::string);

private:
    std::string m_selectedViewId;
    std::string m_hostTransport;
    std::string m_pluginDirectory;
    std::string m_switchViewId;
    std::string m_runtimeConfigPath;
    RuntimeMode m_runtimeMode;
    bool m_runOnce;
    bool m_serve;
    bool m_listViews;
    int m_durationMilliseconds;
    int m_switchViewAfterMilliseconds;
};

bool applyRuntimeConfigArgument(RuntimeConfig&, std::string_view);

} // namespace celestia::runtime
