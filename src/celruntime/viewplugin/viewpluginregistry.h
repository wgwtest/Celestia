// viewpluginregistry.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "viewpluginmanifest.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace celestia::runtime::viewplugin
{

class ViewPluginRegistry
{
public:
    bool registerManifest(ViewPluginManifest);
    const ViewPluginManifest* find(std::string_view id) const;
    std::vector<std::string> viewIds() const;
    bool discover(const std::filesystem::path& directory, std::string* error = nullptr);

private:
    std::map<std::string, ViewPluginManifest> manifests_;
};

ViewPluginRegistry builtinViewPluginRegistry();

} // namespace celestia::runtime::viewplugin
