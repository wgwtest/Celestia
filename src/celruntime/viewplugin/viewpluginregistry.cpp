// viewpluginregistry.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "viewpluginregistry.h"

#include <utility>

namespace celestia::runtime::viewplugin
{
namespace
{

constexpr const char* Debug2DManifest = R"json(
{
  "id": "celestia.view2d.debug",
  "name": "Celestia Debug 2D View",
  "kind": "view2d",
  "version": "0.1.0",
  "protocolVersion": 1,
  "entry": "celestia-view2d-debug-plugin.dll",
  "processMode": "host-process",
  "capabilities": ["view.frame", "view.input", "debug2d"],
  "supportedTransports": ["stdio-pipe"],
  "resourceRequirements": []
}
)json";

constexpr const char* OpenGL3DManifest = R"json(
{
  "id": "celestia.view3d.opengl",
  "name": "Celestia OpenGL 3D View",
  "kind": "view3d",
  "version": "0.1.0",
  "protocolVersion": 1,
  "entry": "celestia-view3d-plugin.dll",
  "processMode": "host-process",
  "capabilities": ["scene.frame", "view.input", "opengl", "sdl-window"],
  "supportedTransports": ["stdio-pipe"],
  "resourceRequirements": ["content-root"]
}
)json";

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

} // end unnamed namespace

bool
ViewPluginRegistry::registerManifest(ViewPluginManifest manifest)
{
    if (manifest.id.empty())
        return false;

    const auto id = manifest.id;
    return manifests_.emplace(id, std::move(manifest)).second;
}

const ViewPluginManifest*
ViewPluginRegistry::find(std::string_view id) const
{
    const auto iter = manifests_.find(std::string(id));
    return iter == manifests_.end() ? nullptr : &iter->second;
}

std::vector<std::string>
ViewPluginRegistry::viewIds() const
{
    std::vector<std::string> ids;
    ids.reserve(manifests_.size());
    for (const auto& [id, manifest] : manifests_)
    {
        (void)manifest;
        ids.push_back(id);
    }
    return ids;
}

bool
ViewPluginRegistry::discover(const std::filesystem::path& directory, std::string* error)
{
    if (directory.empty())
        return true;

    if (!std::filesystem::exists(directory))
    {
        setError(error, "plugin directory does not exist: " + directory.string());
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;

        std::string manifestError;
        auto manifest = loadViewPluginManifest(entry.path(), &manifestError);
        if (!manifest.has_value())
        {
            setError(error, manifestError);
            return false;
        }
        registerManifest(std::move(*manifest));
    }

    return true;
}

ViewPluginRegistry
builtinViewPluginRegistry()
{
    ViewPluginRegistry registry;
    if (auto manifest = parseViewPluginManifest(Debug2DManifest); manifest.has_value())
        registry.registerManifest(std::move(*manifest));
    if (auto manifest = parseViewPluginManifest(OpenGL3DManifest); manifest.has_value())
        registry.registerManifest(std::move(*manifest));
    return registry;
}

} // namespace celestia::runtime::viewplugin
