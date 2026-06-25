// sceneprotocol.h
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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "envelope.h"

namespace celestia::runtime::protocol
{

inline constexpr const char* SceneFrameMessageName{ "scene.frame" };

struct ResourceRef
{
    std::string kind;
    std::string package;
    std::string relativePath;
    std::string contentHash;
};

struct CameraState
{
    std::array<double, 3> position{ 0.0, 0.0, 0.0 };
    std::array<double, 4> orientation{ 1.0, 0.0, 0.0, 0.0 };
    double fov{ 0.0 };
    double nearPlane{ 0.0 };
    double farPlane{ 0.0 };
};

struct ObserverState
{
    std::string referenceBodyId;
    std::string frame;
    std::array<double, 3> position{ 0.0, 0.0, 0.0 };
    std::array<double, 3> velocity{ 0.0, 0.0, 0.0 };
};

struct BodyRenderState
{
    std::string bodyId;
    std::string name;
    std::array<double, 16> transform{
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    double radius{ 0.0 };
    bool visible{ false };
    ResourceRef meshResource;
    ResourceRef diffuseTexture;
    ResourceRef normalTexture;
    std::string material;
};

struct StarRenderState
{
    std::string starId;
    std::array<double, 3> position{ 0.0, 0.0, 0.0 };
    double magnitude{ 0.0 };
    std::array<double, 3> color{ 1.0, 1.0, 1.0 };
    ResourceRef catalogResource;
};

struct OrbitRenderState
{
    std::string bodyId;
    std::vector<std::array<double, 3>> points;
    std::array<double, 4> color{ 1.0, 1.0, 1.0, 1.0 };
    bool visible{ false };
};

struct RenderSettings
{
    bool showStars{ true };
    bool showOrbits{ true };
    bool showLabels{ true };
    double ambientLight{ 0.0 };
    double exposure{ 1.0 };
};

struct SelectionState
{
    std::string type;
    std::string id;
};

struct SceneFrame
{
    std::string sessionId;
    std::uint64_t sequence{ 0 };
    double simulationTime{ 0.0 };
    CameraState camera;
    ObserverState observer;
    RenderSettings renderSettings;
    std::vector<ResourceRef> resources;
    std::vector<BodyRenderState> bodies;
    std::vector<StarRenderState> stars;
    std::vector<OrbitRenderState> orbits;
    std::vector<std::string> labels;
    SelectionState selection;
};

std::string serializeSceneFrame(const SceneFrame&);
std::optional<SceneFrame> deserializeSceneFrame(std::string_view);

RuntimeEnvelope sceneFrameEnvelope(const SceneFrame&, RuntimeRole source, RuntimeRole target);

} // namespace celestia::runtime::protocol
