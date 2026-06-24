// nebularenderassetloader.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>

class Nebula;

namespace celestia::engine
{
class GeometryPaths;
}

namespace celestia::util
{
class AssociativeArray;
}

class NebulaRenderAssetLoader
{
public:
    static bool load(Nebula&,
                     const celestia::util::AssociativeArray*,
                     const std::filesystem::path&,
                     celestia::engine::GeometryPaths&);
};
