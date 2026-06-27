// sceneextractor.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sceneextractor.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

namespace celestia::runtime::model
{
namespace
{

constexpr double Pi = 3.14159265358979323846;

using protocol::BodyRenderState;
using protocol::LabelRenderState;
using protocol::OrbitRenderState;
using protocol::ResourceRef;
using protocol::SceneFrame;
using protocol::StarRenderState;

ResourceRef
resource(std::string kind, std::string package, std::string relativePath)
{
    ResourceRef ref;
    ref.kind = std::move(kind);
    ref.package = std::move(package);
    ref.relativePath = std::move(relativePath);
    ref.id = "res:" + ref.kind + ":" + ref.relativePath;
    return ref;
}

std::string
bodyObjectId(std::string_view id)
{
    if (id.find("celestia:") == 0)
        return std::string(id);
    return "celestia:body:" + std::string(id.empty() ? std::string_view{ "SyntheticBody" } : id);
}

std::array<double, 16>
identityTransform()
{
    return {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
}

BodyRenderState
bodyFromSelection(const ViewFrameSelection& selection)
{
    BodyRenderState body;
    body.bodyId = selection.id.empty() ? "SyntheticBody" : selection.id;
    body.objectId = bodyObjectId(body.bodyId);
    body.name = body.bodyId;
    body.visible = selection.visible;
    body.radius = 6371.0;
    body.meshResource = resource("mesh", "celestia-core", "models/placeholders/sphere.mesh");
    body.diffuseTexture = resource("texture", "celestia-core", "textures/placeholders/body-diffuse.png");
    body.normalTexture = resource("texture", "celestia-core", "textures/placeholders/body-normal.png");
    body.material = "step8-placeholder";

    body.transform = identityTransform();
    body.transform[12] = selection.positionKm[0];
    body.transform[13] = selection.positionKm[1];
    body.transform[14] = selection.positionKm[2];
    return body;
}

BodyRenderState
fallbackBody(double time)
{
    ViewFrameSelection selection;
    selection.type = "body";
    selection.id = "SyntheticEarth";
    selection.visible = true;
    selection.clickable = true;
    selection.positionKm = { std::cos(time * 0.05) * 2.0,
                             std::sin(time * 0.05) * 2.0,
                             0.0 };
    return bodyFromSelection(selection);
}

OrbitRenderState
orbitForBody(std::string bodyId)
{
    OrbitRenderState orbit;
    orbit.bodyId = std::move(bodyId);
    orbit.objectId = "celestia:orbit:" + orbit.bodyId;
    orbit.visible = true;
    orbit.color = { 0.4, 0.7, 1.0, 1.0 };

    constexpr std::size_t PointCount = 48;
    orbit.points.reserve(PointCount);
    for (std::size_t i = 0; i < PointCount; ++i)
    {
        const auto angle = (static_cast<double>(i) / static_cast<double>(PointCount)) * 2.0 * Pi;
        orbit.points.push_back({ std::cos(angle) * 2.0, std::sin(angle) * 2.0, 0.0 });
    }

    return orbit;
}

StarRenderState
solPlaceholder()
{
    StarRenderState star;
    star.starId = "Sol";
    star.objectId = "celestia:star:Sol";
    star.position = { 0.0, 0.0, 0.0 };
    star.magnitude = -26.74;
    star.color = { 1.0, 0.95, 0.82 };
    star.catalogResource = resource("catalog", "celestia-core", "stars/sol-placeholder.stc");
    return star;
}

} // end unnamed namespace

protocol::SceneFrame
extractSceneFrame(std::string_view sessionId, const ViewFrame& snapshot)
{
    SceneFrame frame;
    frame.sessionId = std::string(sessionId);
    frame.sequence = snapshot.frameId;
    frame.simulationTime = snapshot.time;
    frame.time.julianDayTdb = snapshot.time;
    frame.time.secondsSinceJ2000 = 0.0;
    frame.time.timeScale = 1.0;
    frame.time.paused = false;

    frame.camera.position = { 0.0, 0.0, 8.0 };
    frame.camera.orientation = { 0.0, 0.0, 0.0, 1.0 };
    frame.camera.fov = 45.0;
    frame.camera.nearPlane = 0.01;
    frame.camera.farPlane = 1.0e9;

    frame.observer.referenceBodyId = "Sol";
    frame.observer.frame = "step8-synthetic-ecliptic";
    frame.observer.position = { 0.0, 0.0, 8.0 };
    frame.observer.velocity = { 0.0, 0.0, 0.0 };

    frame.renderSettings.showStars = true;
    frame.renderSettings.showOrbits = true;
    frame.renderSettings.showLabels = true;
    frame.renderSettings.ambientLight = 0.35;
    frame.renderSettings.exposure = 1.0;

    frame.resources.push_back(resource("catalog", "celestia-core", "stars/sol-placeholder.stc"));
    frame.resources.push_back(resource("mesh", "celestia-core", "models/placeholders/sphere.mesh"));
    frame.resources.push_back(resource("texture", "celestia-core", "textures/placeholders/body-diffuse.png"));

    for (const auto& selection : snapshot.selections)
    {
        if (selection.type == "body" && selection.visible)
            frame.bodies.push_back(bodyFromSelection(selection));
    }

    if (frame.bodies.empty())
        frame.bodies.push_back(fallbackBody(snapshot.time));

    frame.stars.push_back(solPlaceholder());
    frame.orbits.push_back(orbitForBody(frame.bodies.front().bodyId));
    LabelRenderState label;
    label.targetObjectId = frame.bodies.front().objectId;
    label.text = frame.bodies.front().name;
    label.kind = "body";
    label.visible = true;
    frame.labels.push_back(std::move(label));

    frame.selection.type = "body";
    frame.selection.id = frame.bodies.front().bodyId;
    return frame;
}

} // namespace celestia::runtime::model
