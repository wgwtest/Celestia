// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <unordered_map>

#include <celengine/resource/geometrypaths.h>
#include <celengine/view3d/geometry.h>
#include <celengine/view3d/texmanager.h>

namespace celestia::engine
{

class GeometryManager
{
public:
    GeometryManager(std::shared_ptr<const GeometryPaths>, std::shared_ptr<TexturePaths>);

    const Geometry* find(GeometryHandle);

private:
    std::shared_ptr<const GeometryPaths> m_geometryPaths;
    std::shared_ptr<TexturePaths> m_texturePaths;
    std::unordered_map<GeometryHandle, std::unique_ptr<const Geometry>> m_geometry;
};

class RenderGeometryManager
{
public:
    explicit RenderGeometryManager(std::shared_ptr<GeometryManager>);

    GeometryManager* geometryManager() const noexcept { return m_geometryManager.get(); }
    RenderGeometry* find(GeometryHandle);

private:
    std::shared_ptr<GeometryManager> m_geometryManager;
    std::unordered_map<GeometryHandle, std::unique_ptr<RenderGeometry>> m_geometry;
};

} // end namespace celestia::engine
