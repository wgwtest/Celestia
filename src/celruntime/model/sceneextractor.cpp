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

ResourceRef
resourceFromViewFrame(const ViewFrameResource& source)
{
    ResourceRef ref;
    ref.id = source.id;
    ref.kind = source.kind;
    ref.package = source.package;
    ref.relativePath = source.relativePath;
    ref.contentHash = source.contentHash;
    ref.dataPlaneKey = source.dataPlaneKey;
    ref.required = source.required;
    return ref;
}

const ViewFrameResource*
findResource(const ViewFrame& snapshot, std::string_view id)
{
    if (id.empty())
        return nullptr;

    for (const auto& resource : snapshot.resources)
    {
        if (resource.id == id)
            return &resource;
    }

    return nullptr;
}

ResourceRef
resolveResource(const ViewFrame& snapshot, std::string_view id)
{
    const auto* source = findResource(snapshot, id);
    return source == nullptr ? ResourceRef{} : resourceFromViewFrame(*source);
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
bodyFromProjection(const ViewFrame& snapshot, const ViewFrameBody& source)
{
    BodyRenderState body;
    body.bodyId = source.bodyId.empty() ? source.name : source.bodyId;
    body.objectId = source.objectId.empty() ? bodyObjectId(body.bodyId) : source.objectId;
    body.name = source.name.empty() ? body.bodyId : source.name;
    body.visible = source.visible;
    body.radius = source.radiusKm;
    body.meshResource = resolveResource(snapshot, source.meshResourceId);
    body.diffuseTexture = resolveResource(snapshot, source.diffuseTextureResourceId);
    body.normalTexture = resolveResource(snapshot, source.normalTextureResourceId);
    body.material = source.material.empty() ? "celestia:body" : source.material;

    body.transform = identityTransform();
    body.transform[12] = source.positionKm[0];
    body.transform[13] = source.positionKm[1];
    body.transform[14] = source.positionKm[2];
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

OrbitRenderState
orbitFromProjection(const ViewFrameOrbit& source)
{
    OrbitRenderState orbit;
    orbit.objectId = source.objectId;
    orbit.bodyId = source.bodyId;
    orbit.visible = source.visible;
    orbit.color = source.color;
    orbit.points = source.pointsKm;
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

StarRenderState
starFromProjection(const ViewFrame& snapshot, const ViewFrameStar& source)
{
    StarRenderState star;
    star.starId = source.starId.empty() ? source.name : source.starId;
    star.objectId = source.objectId.empty() ? "celestia:star:" + star.starId : source.objectId;
    star.position = source.positionKm;
    star.magnitude = source.magnitude;
    star.color = source.color;
    star.catalogResource = resolveResource(snapshot, source.catalogResourceId);
    return star;
}

bool
hasProjectedScene(const ViewFrame& snapshot)
{
    return !snapshot.resources.empty() ||
           !snapshot.bodies.empty() ||
           !snapshot.stars.empty() ||
           !snapshot.orbits.empty() ||
           !snapshot.observer.frame.empty() ||
           snapshot.camera.fovDeg > 0.0;
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
    frame.time.secondsSinceJ2000 = (snapshot.time - 2451545.0) * 86400.0;
    frame.time.timeScale = snapshot.timeScale;
    frame.time.paused = snapshot.paused;

    frame.renderSettings.showStars = true;
    frame.renderSettings.showOrbits = true;
    frame.renderSettings.showLabels = true;
    frame.renderSettings.ambientLight = 0.35;
    frame.renderSettings.exposure = 1.0;

    if (hasProjectedScene(snapshot))
    {
        frame.camera.position = snapshot.camera.positionKm;
        frame.camera.orientation = snapshot.camera.orientation;
        frame.camera.fov = snapshot.camera.fovDeg > 0.0 ? snapshot.camera.fovDeg : 45.0;
        frame.camera.nearPlane = snapshot.camera.nearPlaneKm > 0.0 ? snapshot.camera.nearPlaneKm : 0.01;
        frame.camera.farPlane = snapshot.camera.farPlaneKm > frame.camera.nearPlane
            ? snapshot.camera.farPlaneKm
            : 1.0e9;

        frame.observer.referenceBodyId = snapshot.observer.referenceBodyId;
        frame.observer.frame = snapshot.observer.frame.empty()
            ? "celestia:observer:universal"
            : snapshot.observer.frame;
        frame.observer.position = snapshot.observer.positionKm;
        frame.observer.velocity = snapshot.observer.velocityKmPerSec;

        for (const auto& source : snapshot.resources)
            frame.resources.push_back(resourceFromViewFrame(source));

        for (const auto& source : snapshot.bodies)
            frame.bodies.push_back(bodyFromProjection(snapshot, source));

        for (const auto& source : snapshot.stars)
            frame.stars.push_back(starFromProjection(snapshot, source));

        for (const auto& source : snapshot.orbits)
            frame.orbits.push_back(orbitFromProjection(source));

        for (const auto& body : frame.bodies)
        {
            LabelRenderState label;
            label.targetObjectId = body.objectId;
            label.text = body.name;
            label.kind = "body";
            label.visible = body.visible;
            frame.labels.push_back(std::move(label));
        }

        if (!snapshot.selections.empty())
        {
            frame.selection.type = snapshot.selections.front().type;
            frame.selection.id = snapshot.selections.front().id;
        }
        else if (!frame.bodies.empty())
        {
            frame.selection.type = "body";
            frame.selection.id = frame.bodies.front().bodyId;
        }
        else if (!frame.stars.empty())
        {
            frame.selection.type = "star";
            frame.selection.id = frame.stars.front().starId;
        }

        return frame;
    }

    frame.camera.position = { 0.0, 0.0, 8.0 };
    frame.camera.orientation = { 0.0, 0.0, 0.0, 1.0 };
    frame.camera.fov = 45.0;
    frame.camera.nearPlane = 0.01;
    frame.camera.farPlane = 1.0e9;

    frame.observer.referenceBodyId = "Sol";
    frame.observer.frame = "step8-synthetic-ecliptic";
    frame.observer.position = { 0.0, 0.0, 8.0 };
    frame.observer.velocity = { 0.0, 0.0, 0.0 };

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
