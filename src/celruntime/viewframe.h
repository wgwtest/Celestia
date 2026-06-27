// viewframe.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace celestia::runtime
{

struct ViewFrameSelection
{
    std::string type;
    std::string id;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    bool visible{ false };
    bool clickable{ false };
};

struct ViewFrameResource
{
    std::string id;
    std::string kind;
    std::string package;
    std::string relativePath;
    std::string contentHash;
    std::string dataPlaneKey;
    bool required{ false };
};

struct ViewFrameCamera
{
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    std::array<double, 4> orientation{ 0.0, 0.0, 0.0, 1.0 };
    double fovDeg{ 0.0 };
    double nearPlaneKm{ 0.0 };
    double farPlaneKm{ 0.0 };
};

struct ViewFrameObserver
{
    std::string referenceBodyId;
    std::string frame;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    std::array<double, 3> velocityKmPerSec{ 0.0, 0.0, 0.0 };
};

struct ViewFrameBody
{
    std::string objectId;
    std::string bodyId;
    std::string name;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    double radiusKm{ 0.0 };
    bool visible{ false };
    std::string meshResourceId;
    std::string diffuseTextureResourceId;
    std::string normalTextureResourceId;
    std::string material;
};

struct ViewFrameStar
{
    std::string objectId;
    std::string starId;
    std::string name;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    double magnitude{ 0.0 };
    std::array<double, 3> color{ 1.0, 1.0, 1.0 };
    std::string catalogResourceId;
};

struct ViewFrameOrbit
{
    std::string objectId;
    std::string bodyId;
    std::vector<std::array<double, 3>> pointsKm;
    std::array<double, 4> color{ 0.4, 0.7, 1.0, 1.0 };
    bool visible{ false };
};

struct ViewFrame
{
    std::uint64_t frameId{ 0 };
    double time{ 0.0 };
    double timeScale{ 1.0 };
    bool paused{ false };
    ViewFrameCamera camera;
    ViewFrameObserver observer;
    std::vector<ViewFrameResource> resources;
    std::vector<ViewFrameBody> bodies;
    std::vector<ViewFrameStar> stars;
    std::vector<ViewFrameOrbit> orbits;
    std::vector<ViewFrameSelection> selections;
    std::string summary;
};

} // namespace celestia::runtime
