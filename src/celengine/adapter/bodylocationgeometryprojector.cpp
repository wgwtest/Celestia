// bodylocationgeometryprojector.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/bodylocationgeometryprojector.h>

#include <cassert>

#include <Eigen/Geometry>

#include <celmath/ray.h>
#include <celengine/model/body.h>
#include <celengine/adapter/bodyrenderassets.h>
#include <celengine/view3d/geometry.h>
#include <celengine/model/location.h>
#include <celengine/view3d/meshmanager.h>

namespace engine = celestia::engine;
namespace math = celestia::math;

// Compute the positions of locations on an irregular object using ray-mesh
// intersections.  This is not automatically done when a location is added
// because it would force the loading of all meshes for objects with
// defined locations; on-demand loading of meshes is preferred.
void
BodyLocationGeometryProjector::computeLocations(const Body* body,
                                                BodyFeaturesManager& bodyFeaturesManager,
                                                engine::GeometryManager& geometryManager)
{
    if (!bodyFeaturesManager.hasLocations(body))
        return;

    auto it = bodyFeaturesManager.locations.find(body);
    assert(it != bodyFeaturesManager.locations.end());

    auto& bodyLocations = it->second;
    if (bodyLocations.locationsComputed)
        return;

    bodyLocations.locationsComputed = true;

    // No work to do if there's no mesh, or if the mesh cannot be loaded
    auto geometry = BodyRenderAssets::getGeometry(body);
    if (geometry == engine::GeometryHandle::Invalid)
        return;

    const Geometry* g = geometryManager.find(geometry);
    if (g == nullptr)
        return;

    // TODO: Implement separate radius and bounding radius so that this hack is
    // not necessary.
    constexpr float boundingRadius = 2.0f;
    auto radius = body->getRadius();
    Eigen::Quaterniond orientation = BodyRenderAssets::getGeometryOrientation(body).cast<double>();
    for (const auto& location : bodyLocations.locations)
    {
        Location* loc = location.get();
        Eigen::Vector3f v = loc->getPosition();
        float alt = v.norm() - radius;
        if (alt > 0.1f * radius) // assume we don't have locations with height > 0.1*radius
            continue;
        if (alt != -radius)
            v.normalize();
        v *= boundingRadius;

        Eigen::ParametrizedLine<double, 3> ray(v.cast<double>(), -v.cast<double>());
        double t = 0.0;
        if (g->pick(math::transformRay(ray, orientation), t))
        {
            v *= static_cast<float>((1.0 - t) * radius + alt);
            loc->setPosition(v);
        }
    }
}
