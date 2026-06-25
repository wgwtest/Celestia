// bodyrenderassets.h
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
#include <string_view>
#include <vector>

#include <Eigen/Geometry>

#include <celutil/texhandle.h>
#include <celengine/view3d/meshmanager.h>
#include <celengine/model/surface.h>

class Body;
struct RingSystem;

class BodyRenderAssets
{
public:
    static celestia::engine::GeometryHandle getGeometry(const Body*);
    static void setGeometry(const Body*, celestia::engine::GeometryHandle);

    static Eigen::Quaternionf getGeometryOrientation(const Body*);
    static void setGeometryOrientation(const Body*, const Eigen::Quaternionf&);

    static float getGeometryScale(const Body*);
    static void setGeometryScale(const Body*, float);

    static const Surface& getSurface(const Body*);
    static void setSurface(const Body*, const Surface&);

    static Surface* getAlternateSurface(const Body*, std::string_view);
    static void setAlternateSurface(const Body*, std::string_view, std::unique_ptr<Surface>&&);
    static std::vector<std::string> getAlternateSurfaceNames(const Body*);

    static celestia::util::TextureHandle getRingTexture(const RingSystem*);
    static void setRingTexture(const RingSystem*, celestia::util::TextureHandle);
    static void removeRing(const RingSystem*);

    static void reset(const Body*);
    static void remove(const Body*);
};
