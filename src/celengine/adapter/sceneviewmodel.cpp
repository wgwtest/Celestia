// sceneviewmodel.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/sceneviewmodel.h>

#include <array>
#include <cmath>
#include <string>
#include <utility>

#include <celengine/model/body.h>
#include <celengine/model/deepskyobj.h>
#include <celengine/model/location.h>
#include <celengine/controller/observer.h>
#include <celengine/controller/selection.h>
#include <celengine/controller/simulation.h>
#include <celengine/model/star.h>
#include <celengine/model/stardb.h>
#include <celengine/model/univcoord.h>
#include <celengine/model/universe.h>

namespace
{

constexpr double DegreesPerRadian = 57.2957795130823208768;

bool
isClickable(const Selection& selection)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr;
    case SelectionType::Body:
        return selection.body() != nullptr && selection.body()->isClickable();
    case SelectionType::DeepSky:
        return selection.deepsky() != nullptr && selection.deepsky()->isClickable();
    default:
        return false;
    }
}

std::string
selectionTypeName(SelectionType type)
{
    switch (type)
    {
    case SelectionType::Star:
        return "star";
    case SelectionType::Body:
        return "body";
    case SelectionType::DeepSky:
        return "deepsky";
    case SelectionType::Location:
        return "location";
    default:
        return "none";
    }
}

std::string
bodyPath(const Body& body, const StarDatabase* starCatalog)
{
    auto path = body.getPath(starCatalog, '/');
    return path.empty() ? body.getName(false) : path;
}

std::string
starName(const Star& star, const StarDatabase* starCatalog)
{
    return starCatalog == nullptr ? std::to_string(star.getIndex()) : starCatalog->getStarName(star);
}

std::string
objectIdForBody(const Body& body, const StarDatabase* starCatalog)
{
    return "celestia:body:" + bodyPath(body, starCatalog);
}

std::string
objectIdForStar(const Star& star)
{
    return "celestia:star:" + std::to_string(star.getIndex());
}

std::string
objectIdForSelection(const Selection& selection, const StarDatabase* starCatalog)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr ? objectIdForStar(*selection.star()) : std::string();
    case SelectionType::Body:
        return selection.body() != nullptr ? objectIdForBody(*selection.body(), starCatalog) : std::string();
    case SelectionType::DeepSky:
        return selection.deepsky() != nullptr
            ? "celestia:dso:" + std::to_string(selection.deepsky()->getIndex())
            : std::string();
    case SelectionType::Location:
        return selection.location() != nullptr ? "celestia:location:" + selection.location()->getName(false) : std::string();
    default:
        return {};
    }
}

std::string
selectionId(const Selection& selection)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr ? std::to_string(selection.star()->getIndex()) : std::string();
    case SelectionType::Body:
        return selection.body() != nullptr ? selection.body()->getName(false) : std::string();
    case SelectionType::DeepSky:
        return selection.deepsky() != nullptr ? std::to_string(selection.deepsky()->getIndex()) : std::string();
    case SelectionType::Location:
        return selection.location() != nullptr ? selection.location()->getName(false) : std::string();
    default:
        return {};
    }
}

std::string
selectionId(const Selection& selection, const StarDatabase* starCatalog)
{
    switch (selection.getType())
    {
    case SelectionType::Star:
        return selection.star() != nullptr ? std::to_string(selection.star()->getIndex()) : std::string();
    case SelectionType::Body:
        return selection.body() != nullptr ? bodyPath(*selection.body(), starCatalog) : std::string();
    default:
        return selectionId(selection);
    }
}

std::array<double, 3>
toPositionKm(const Eigen::Vector3d& positionKm)
{
    return { positionKm.x(), positionKm.y(), positionKm.z() };
}

std::array<double, 3>
toPositionKm(const UniversalCoord& position)
{
    return toPositionKm(position.offsetFromKm(UniversalCoord::Zero()));
}

std::array<double, 4>
toOrientation(const Eigen::Quaterniond& orientation)
{
    return { orientation.x(), orientation.y(), orientation.z(), orientation.w() };
}

std::string
coordinateSystemName(ObserverFrame::CoordinateSystem coordinateSystem)
{
    switch (coordinateSystem)
    {
    case ObserverFrame::CoordinateSystem::Universal:
        return "celestia:observer:universal";
    case ObserverFrame::CoordinateSystem::Ecliptical:
        return "celestia:observer:ecliptical";
    case ObserverFrame::CoordinateSystem::Equatorial:
        return "celestia:observer:equatorial";
    case ObserverFrame::CoordinateSystem::BodyFixed:
        return "celestia:observer:body-fixed";
    case ObserverFrame::CoordinateSystem::PhaseLock:
        return "celestia:observer:phase-lock";
    case ObserverFrame::CoordinateSystem::Chase:
        return "celestia:observer:chase";
    case ObserverFrame::CoordinateSystem::ObserverLocal:
        return "celestia:observer:local";
    default:
        return "celestia:observer:unknown";
    }
}

void
appendStar(SceneViewSnapshot& snapshot, const Star& star, const StarDatabase* starCatalog, double time)
{
    const auto id = std::to_string(star.getIndex());
    for (const auto& existing : snapshot.stars)
    {
        if (existing.starId == id)
            return;
    }

    celestia::runtime::ViewFrameStar starSnapshot;
    starSnapshot.objectId = objectIdForStar(star);
    starSnapshot.starId = id;
    starSnapshot.name = starName(star, starCatalog);
    starSnapshot.positionKm = toPositionKm(star.getPosition(time));
    starSnapshot.magnitude = star.getAbsoluteMagnitude();
    starSnapshot.color = { 1.0, 0.95, 0.82 };
    starSnapshot.catalogResourceId = "res:catalog:stars";
    snapshot.stars.push_back(std::move(starSnapshot));
}

void
appendOrbit(SceneViewSnapshot& snapshot, const Body& body, const StarDatabase* starCatalog, double time)
{
    if (body.getOrbit(time) == nullptr)
        return;

    const auto path = bodyPath(body, starCatalog);
    celestia::runtime::ViewFrameOrbit orbit;
    orbit.objectId = "celestia:orbit:" + path;
    orbit.bodyId = path;
    orbit.visible = true;

    constexpr int Samples = 16;
    constexpr double Days = 1.0;
    orbit.pointsKm.reserve(Samples);
    for (int i = 0; i < Samples; ++i)
    {
        const auto alpha = Samples == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(Samples - 1);
        const auto sampleTime = time + (alpha - 0.5) * Days;
        orbit.pointsKm.push_back(toPositionKm(body.getPosition(sampleTime)));
    }

    snapshot.orbits.push_back(std::move(orbit));
}

void
appendBody(SceneViewSnapshot& snapshot, const Body& body, const StarDatabase* starCatalog, double time)
{
    const auto path = bodyPath(body, starCatalog);
    for (const auto& existing : snapshot.bodies)
    {
        if (existing.bodyId == path)
            return;
    }

    celestia::runtime::ViewFrameBody bodySnapshot;
    bodySnapshot.objectId = objectIdForBody(body, starCatalog);
    bodySnapshot.bodyId = path;
    bodySnapshot.name = body.getName(false);
    bodySnapshot.positionKm = toPositionKm(body.getPosition(time));
    bodySnapshot.radiusKm = body.getRadius();
    bodySnapshot.visible = body.isVisible();
    bodySnapshot.material = "celestia:body";
    snapshot.bodies.push_back(std::move(bodySnapshot));
    appendOrbit(snapshot, body, starCatalog, time);

    if (body.getSystem() != nullptr && body.getSystem()->getStar() != nullptr)
        appendStar(snapshot, *body.getSystem()->getStar(), starCatalog, time);
}

} // end unnamed namespace

SceneViewSnapshot
SceneViewModel::buildSelectionSnapshot(const Simulation& simulation)
{
    SceneViewSnapshot snapshot;
    snapshot.time = simulation.getTime();
    snapshot.timeScale = simulation.getTimeScale();
    snapshot.paused = simulation.getPauseState();

    const auto* universe = simulation.getUniverse();
    const auto* starCatalog = universe == nullptr ? nullptr : universe->getStarCatalog();

    if (const auto* observer = simulation.getActiveObserver(); observer != nullptr)
    {
        snapshot.camera.positionKm = toPositionKm(observer->getPosition());
        snapshot.camera.orientation = toOrientation(observer->getOrientation());
        snapshot.camera.fovDeg = static_cast<double>(observer->getFOV()) * DegreesPerRadian;
        snapshot.camera.nearPlaneKm = 0.001;
        snapshot.camera.farPlaneKm = 1.0e9;

        snapshot.observer.positionKm = snapshot.camera.positionKm;
        snapshot.observer.velocityKmPerSec = toPositionKm(observer->getVelocity());
        if (observer->getFrame() != nullptr)
        {
            snapshot.observer.referenceBodyId = objectIdForSelection(observer->getFrame()->getRefObject(), starCatalog);
            snapshot.observer.frame = coordinateSystemName(observer->getFrame()->getCoordinateSystem());
        }
    }

    Selection selection = simulation.getSelection();
    if (!selection.empty())
    {
        SceneSelectionSnapshot selectionSnapshot;
        selectionSnapshot.type = selectionTypeName(selection.getType());
        selectionSnapshot.id = selectionId(selection, starCatalog);
        selectionSnapshot.positionKm = toPositionKm(selection.getPosition(snapshot.time).offsetFromKm(UniversalCoord::Zero()));
        selectionSnapshot.visible = selection.isVisible();
        selectionSnapshot.clickable = isClickable(selection);
        snapshot.selections.push_back(selectionSnapshot);

        if (selection.body() != nullptr)
            appendBody(snapshot, *selection.body(), starCatalog, snapshot.time);
        if (selection.star() != nullptr)
            appendStar(snapshot, *selection.star(), starCatalog, snapshot.time);
    }

    return snapshot;
}
