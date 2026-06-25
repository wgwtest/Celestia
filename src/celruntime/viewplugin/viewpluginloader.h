// viewpluginloader.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "viewpluginabi.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace celestia::runtime::viewplugin
{

class ViewPluginProviderHandle
{
public:
    ViewPluginProviderHandle() = default;
    ViewPluginProviderHandle(CelestiaViewProvider*, CelestiaDestroyViewProviderFn);
    ~ViewPluginProviderHandle();

    ViewPluginProviderHandle(const ViewPluginProviderHandle&) = delete;
    ViewPluginProviderHandle& operator=(const ViewPluginProviderHandle&) = delete;
    ViewPluginProviderHandle(ViewPluginProviderHandle&&) noexcept;
    ViewPluginProviderHandle& operator=(ViewPluginProviderHandle&&) noexcept;

    bool valid() const;

private:
    CelestiaViewProvider* provider_{ nullptr };
    CelestiaDestroyViewProviderFn destroy_{ nullptr };
};

class LoadedViewPlugin
{
public:
    LoadedViewPlugin() = default;
    ~LoadedViewPlugin();

    LoadedViewPlugin(const LoadedViewPlugin&) = delete;
    LoadedViewPlugin& operator=(const LoadedViewPlugin&) = delete;
    LoadedViewPlugin(LoadedViewPlugin&&) noexcept;
    LoadedViewPlugin& operator=(LoadedViewPlugin&&) noexcept;

    bool valid() const;
    int abiVersion() const;
    const std::string& id() const;
    ViewPluginProviderHandle createProvider(std::string* error = nullptr) const;

private:
    friend class ViewPluginLoader;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class ViewPluginLoader
{
public:
    std::optional<LoadedViewPlugin> load(const std::filesystem::path&, std::string* error = nullptr) const;
};

} // namespace celestia::runtime::viewplugin
