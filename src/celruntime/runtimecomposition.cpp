// runtimecomposition.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimecomposition.h"

#include "ipc/localchannel.h"

#include <utility>

namespace celestia::runtime
{

RuntimeComposition::RuntimeComposition()
{
    configureChannel();
}

RuntimeComposition::RuntimeComposition(RuntimeConfig config) :
    m_config(std::move(config))
{
    configureChannel();
}

const RuntimeConfig&
RuntimeComposition::config() const
{
    return m_config;
}

RuntimeConfig&
RuntimeComposition::config()
{
    return m_config;
}

const std::string&
RuntimeComposition::selectedViewId() const
{
    return m_config.selectedViewId();
}

RuntimeMode
RuntimeComposition::runtimeMode() const
{
    return m_config.runtimeMode();
}

const ModelRuntime&
RuntimeComposition::modelRuntime() const
{
    return m_modelRuntime;
}

ModelRuntime&
RuntimeComposition::modelRuntime()
{
    return m_modelRuntime;
}

const ControllerRuntime&
RuntimeComposition::controllerRuntime() const
{
    return m_controllerRuntime;
}

ControllerRuntime&
RuntimeComposition::controllerRuntime()
{
    return m_controllerRuntime;
}

const ViewAdapterRuntime&
RuntimeComposition::viewAdapterRuntime() const
{
    return m_viewAdapterRuntime;
}

ViewAdapterRuntime&
RuntimeComposition::viewAdapterRuntime()
{
    return m_viewAdapterRuntime;
}

const ViewProviderRegistry&
RuntimeComposition::viewProviders() const
{
    return m_viewProviders;
}

ViewProviderRegistry&
RuntimeComposition::viewProviders()
{
    return m_viewProviders;
}

const ipc::Channel*
RuntimeComposition::runtimeChannel() const
{
    return m_runtimeChannel.get();
}

ipc::Channel*
RuntimeComposition::runtimeChannel()
{
    return m_runtimeChannel.get();
}

void
RuntimeComposition::configureChannel()
{
    if (m_config.runtimeMode() == RuntimeMode::InProcessChannel)
        m_runtimeChannel = std::make_unique<ipc::LocalChannel>();
    else
        m_runtimeChannel.reset();
}

} // namespace celestia::runtime
