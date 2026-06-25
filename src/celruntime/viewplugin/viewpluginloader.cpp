// viewpluginloader.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "viewpluginloader.h"

#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace celestia::runtime::viewplugin
{
namespace
{

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

} // end unnamed namespace

ViewPluginProviderHandle::ViewPluginProviderHandle(CelestiaViewProvider* provider,
                                                   CelestiaDestroyViewProviderFn destroy)
    : provider_(provider),
      destroy_(destroy)
{
}

ViewPluginProviderHandle::~ViewPluginProviderHandle()
{
    if (provider_ != nullptr && destroy_ != nullptr)
        destroy_(provider_);
}

ViewPluginProviderHandle::ViewPluginProviderHandle(ViewPluginProviderHandle&& other) noexcept
    : provider_(std::exchange(other.provider_, nullptr)),
      destroy_(std::exchange(other.destroy_, nullptr))
{
}

ViewPluginProviderHandle&
ViewPluginProviderHandle::operator=(ViewPluginProviderHandle&& other) noexcept
{
    if (this != &other)
    {
        if (provider_ != nullptr && destroy_ != nullptr)
            destroy_(provider_);
        provider_ = std::exchange(other.provider_, nullptr);
        destroy_ = std::exchange(other.destroy_, nullptr);
    }
    return *this;
}

bool
ViewPluginProviderHandle::valid() const
{
    return provider_ != nullptr;
}

struct LoadedViewPlugin::Impl
{
#ifdef _WIN32
    HMODULE library{ nullptr };
#endif
    int abiVersion{ 0 };
    std::string id;
    CelestiaCreateViewProviderFn createProvider{ nullptr };
    CelestiaDestroyViewProviderFn destroyProvider{ nullptr };

    ~Impl()
    {
#ifdef _WIN32
        if (library != nullptr)
            FreeLibrary(library);
#endif
    }
};

LoadedViewPlugin::~LoadedViewPlugin() = default;
LoadedViewPlugin::LoadedViewPlugin(LoadedViewPlugin&&) noexcept = default;
LoadedViewPlugin& LoadedViewPlugin::operator=(LoadedViewPlugin&&) noexcept = default;

bool
LoadedViewPlugin::valid() const
{
    return impl_ != nullptr;
}

int
LoadedViewPlugin::abiVersion() const
{
    return impl_ == nullptr ? 0 : impl_->abiVersion;
}

const std::string&
LoadedViewPlugin::id() const
{
    static const std::string empty;
    return impl_ == nullptr ? empty : impl_->id;
}

ViewPluginProviderHandle
LoadedViewPlugin::createProvider(std::string* error) const
{
    if (impl_ == nullptr || impl_->createProvider == nullptr || impl_->destroyProvider == nullptr)
    {
        setError(error, "view plugin is not loaded");
        return {};
    }

    auto* provider = impl_->createProvider();
    if (provider == nullptr)
    {
        setError(error, "view plugin returned a null provider");
        return {};
    }

    return ViewPluginProviderHandle(provider, impl_->destroyProvider);
}

std::optional<LoadedViewPlugin>
ViewPluginLoader::load(const std::filesystem::path& path, std::string* error) const
{
#ifndef _WIN32
    (void)path;
    setError(error, "dynamic view plugins are unsupported on this platform in Step9");
    return std::nullopt;
#else
    auto* library = LoadLibraryW(path.wstring().c_str());
    if (library == nullptr)
    {
        setError(error, "failed to load view plugin: " + path.string());
        return std::nullopt;
    }

    auto* abiVersion = reinterpret_cast<CelestiaViewPluginAbiVersionFn>(
        GetProcAddress(library, "celestia_view_plugin_abi_version"));
    auto* pluginId = reinterpret_cast<CelestiaViewPluginIdFn>(
        GetProcAddress(library, "celestia_view_plugin_id"));
    auto* createProvider = reinterpret_cast<CelestiaCreateViewProviderFn>(
        GetProcAddress(library, "celestia_create_view_provider"));
    auto* destroyProvider = reinterpret_cast<CelestiaDestroyViewProviderFn>(
        GetProcAddress(library, "celestia_destroy_view_provider"));

    if (abiVersion == nullptr || pluginId == nullptr ||
        createProvider == nullptr || destroyProvider == nullptr)
    {
        FreeLibrary(library);
        setError(error, "view plugin is missing required ABI entry points");
        return std::nullopt;
    }

    const auto version = abiVersion();
    if (version != CELESTIA_VIEW_PLUGIN_ABI_VERSION)
    {
        FreeLibrary(library);
        setError(error, "view plugin ABI version mismatch");
        return std::nullopt;
    }

    const auto* id = pluginId();
    if (id == nullptr || *id == '\0')
    {
        FreeLibrary(library);
        setError(error, "view plugin id is empty");
        return std::nullopt;
    }

    LoadedViewPlugin loaded;
    loaded.impl_ = std::make_unique<LoadedViewPlugin::Impl>();
    loaded.impl_->library = library;
    loaded.impl_->abiVersion = version;
    loaded.impl_->id = id;
    loaded.impl_->createProvider = createProvider;
    loaded.impl_->destroyProvider = destroyProvider;
    return loaded;
#endif
}

} // namespace celestia::runtime::viewplugin
