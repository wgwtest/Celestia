// viewpluginlifecycle.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

namespace celestia::runtime::viewplugin
{

enum class ViewPluginState
{
    Unloaded,
    Loaded,
    Configured,
    Started,
    Stopping,
    Stopped,
    Shutdown,
    Failed,
};

bool canTransition(ViewPluginState from, ViewPluginState to);

} // namespace celestia::runtime::viewplugin
