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
#include <utility>

namespace celestia::runtime::view3d
{

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

        const std::filesystem::path relative{ resource.relativePath };
        if (!contentRoot.empty() && !relative.empty() && relative.is_relative())
        {
            entry.resolvedPath = contentRoot / relative;
            entry.exists = std::filesystem::exists(entry.resolvedPath);
        }

        resolved.push_back(std::move(entry));
    }

    return resolved;
}

} // namespace celestia::runtime::view3d
