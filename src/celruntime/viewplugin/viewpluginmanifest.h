// viewpluginmanifest.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace celestia::runtime::viewplugin
{

inline constexpr int CurrentViewPluginProtocolVersion{ 1 };

struct ViewPluginManifest
{
    std::string id;
    std::string name;
    std::string kind;
    std::string version;
    int protocolVersion{ 0 };
    std::string entry;
    std::string processMode;
    std::vector<std::string> capabilities;
    std::vector<std::string> supportedTransports;
    std::vector<std::string> resourceRequirements;

    bool supports(std::string_view capability) const;
};

std::optional<ViewPluginManifest> parseViewPluginManifest(std::string_view, std::string* error = nullptr);
std::optional<ViewPluginManifest> loadViewPluginManifest(const std::filesystem::path&, std::string* error = nullptr);

} // namespace celestia::runtime::viewplugin
