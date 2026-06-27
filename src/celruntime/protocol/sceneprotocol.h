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
inline constexpr int SceneFrameProtocolVersion{ 2 };

struct ResourceRef
{
    std::string id;
    std::string kind;
    std::string package;
    std::string relativePath;
    std::string contentHash;
    std::string dataPlaneKey;
    bool required{ false };
};

struct TimeState
{
    double julianDayTdb{ 0.0 };
    double secondsSinceJ2000{ 0.0 };
    double timeScale{ 1.0 };
    bool paused{ false };
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
    std::string objectId;
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
    std::string objectId;
    std::string starId;
    std::array<double, 3> position{ 0.0, 0.0, 0.0 };
    double magnitude{ 0.0 };
    std::array<double, 3> color{ 1.0, 1.0, 1.0 };
    ResourceRef catalogResource;
};

struct OrbitRenderState
{
    std::string objectId;
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

struct LabelRenderState
{
    std::string targetObjectId;
    std::string text;
    std::string kind;
    bool visible{ true };
};

struct DeepSkyRenderState
{
    std::string objectId;
    std::string name;
    std::string dsoType;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    double apparentMagnitude{ 0.0 };
    bool visible{ true };
    ResourceRef catalogResource;
};

struct SceneFrame
{
    int protocolVersion{ SceneFrameProtocolVersion };
    std::string sessionId;
    std::uint64_t sequence{ 0 };
    double simulationTime{ 0.0 };
    TimeState time;
    CameraState camera;
    ObserverState observer;
    RenderSettings renderSettings;
    std::vector<ResourceRef> resources;
    std::vector<BodyRenderState> bodies;
    std::vector<StarRenderState> stars;
    std::vector<DeepSkyRenderState> deepSkyObjects;
    std::vector<OrbitRenderState> orbits;
    std::vector<LabelRenderState> labels;
    SelectionState selection;
};

struct SceneFrameValidationIssue
{
    std::string field;
    std::string message;
};

std::string serializeSceneFrame(const SceneFrame&);
std::optional<SceneFrame> deserializeSceneFrame(std::string_view);
std::vector<SceneFrameValidationIssue> validateSceneFrame(const SceneFrame&);
bool isValidSceneFrame(const SceneFrame&);

RuntimeEnvelope sceneFrameEnvelope(const SceneFrame&, RuntimeRole source, RuntimeRole target);

} // namespace celestia::runtime::protocol
