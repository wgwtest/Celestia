// view3dresources.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dresources.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <celruntime/dataplane/dataplaneref.h>

namespace celestia::runtime::view3d
{
namespace
{

bool
hasWindowsRoot(std::string_view path)
{
    return path.size() >= 2 && path[1] == ':';
}

bool
isSafeResourcePath(std::string_view resourcePath, const std::filesystem::path& relative)
{
    if (resourcePath.empty() || relative.empty() || !relative.is_relative())
        return false;
    if (resourcePath.front() == '/' || resourcePath.front() == '\\' || hasWindowsRoot(resourcePath))
        return false;

    for (const auto& part : relative)
    {
        if (part == "..")
            return false;
    }

    return true;
}

std::string
resourceCacheKey(const protocol::ResourceRef& resource)
{
    if (!resource.dataPlaneKey.empty())
        return resource.dataPlaneKey;
    if (!resource.contentHash.empty())
        return resource.contentHash;
    return resource.package + "|" + resource.kind + "|" + resource.relativePath;
}

bool
isDataPlaneEligibleResourceKind(std::string_view kind)
{
    return kind == "texture" ||
           kind == "mesh" ||
           kind == "catalog" ||
           kind == "orbit-sample" ||
           kind == "label-atlas";
}

} // end unnamed namespace

std::optional<dataplane::DataPlaneRef>
dataPlaneRefFromResource(const protocol::ResourceRef& resource)
{
    if (resource.dataPlaneKey.empty())
        return std::nullopt;
    return dataplane::deserializeDataPlaneRef(resource.dataPlaneKey);
}

std::vector<View3DResolvedResource>
resolveSceneResources(const protocol::SceneFrame& frame,
                      const std::filesystem::path& contentRoot)
{
    std::vector<View3DResolvedResource> resolved;
    resolved.reserve(frame.resources.size());

    for (const auto& resource : frame.resources)
    {
        View3DResolvedResource entry;
        entry.resource = resource;
        entry.cacheKey = resourceCacheKey(resource);
        entry.dataPlaneRef = dataPlaneRefFromResource(resource);
        entry.dataPlaneEligible = isDataPlaneEligibleResourceKind(resource.kind) &&
                                  entry.dataPlaneRef.has_value();

        const std::filesystem::path relative{ resource.relativePath };
        if (!isSafeResourcePath(resource.relativePath, relative))
        {
            entry.status = View3DResourceStatus::Invalid;
            resolved.push_back(std::move(entry));
            continue;
        }

        if (!contentRoot.empty())
        {
            entry.resolvedPath = contentRoot / relative;
            entry.exists = std::filesystem::exists(entry.resolvedPath);
        }

        entry.status = entry.exists
            ? View3DResourceStatus::Resolved
            : (resource.required
                   ? View3DResourceStatus::MissingRequired
                   : View3DResourceStatus::MissingOptional);

        resolved.push_back(std::move(entry));
    }

    return resolved;
}

} // namespace celestia::runtime::view3d
