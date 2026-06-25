// runtimecomposition.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>

#include "controllerruntime.h"
#include "ipc/channel.h"
#include "modelruntime.h"
#include "runtimeconfig.h"
#include "viewadapterruntime.h"
#include "viewproviderregistry.h"

namespace celestia::runtime
{

class RuntimeComposition
{
public:
    RuntimeComposition();
    explicit RuntimeComposition(RuntimeConfig);

    const RuntimeConfig& config() const;
    RuntimeConfig& config();

    const std::string& selectedViewId() const;
    RuntimeMode runtimeMode() const;

    const ModelRuntime& modelRuntime() const;
    ModelRuntime& modelRuntime();

    const ControllerRuntime& controllerRuntime() const;
    ControllerRuntime& controllerRuntime();

    const ViewAdapterRuntime& viewAdapterRuntime() const;
    ViewAdapterRuntime& viewAdapterRuntime();

    const ViewProviderRegistry& viewProviders() const;
    ViewProviderRegistry& viewProviders();

    const ipc::Channel* runtimeChannel() const;
    ipc::Channel* runtimeChannel();

private:
    void configureChannel();

    RuntimeConfig m_config;
    ModelRuntime m_modelRuntime;
    ControllerRuntime m_controllerRuntime;
    ViewAdapterRuntime m_viewAdapterRuntime;
    ViewProviderRegistry m_viewProviders;
    std::unique_ptr<ipc::Channel> m_runtimeChannel;
};

} // namespace celestia::runtime
