// viewpluginlifecycle.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "viewpluginlifecycle.h"

namespace celestia::runtime::viewplugin
{

bool
canTransition(ViewPluginState from, ViewPluginState to)
{
    if (to == ViewPluginState::Failed)
        return true;

    switch (from)
    {
    case ViewPluginState::Unloaded:
        return to == ViewPluginState::Loaded;
    case ViewPluginState::Loaded:
        return to == ViewPluginState::Configured;
    case ViewPluginState::Configured:
        return to == ViewPluginState::Started;
    case ViewPluginState::Started:
        return to == ViewPluginState::Stopping;
    case ViewPluginState::Stopping:
        return to == ViewPluginState::Stopped;
    case ViewPluginState::Stopped:
        return to == ViewPluginState::Shutdown;
    case ViewPluginState::Shutdown:
    case ViewPluginState::Failed:
        return false;
    }

    return false;
}

} // namespace celestia::runtime::viewplugin
