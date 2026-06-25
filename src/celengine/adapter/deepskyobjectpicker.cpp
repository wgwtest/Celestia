// deepskyobjectpicker.cpp
//
// Copyright (C) 2003-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/deepskyobjectpicker.h>

#include <celmath/intersect.h>
#include <celmath/ray.h>
#include <celmath/sphere.h>
#include <celengine/model/deepskyobj.h>
#include <celengine/legacy/galaxy.h>
#include <celengine/legacy/galaxyform.h>
#include <celengine/legacy/globular.h>
#include <celengine/model/opencluster.h>

namespace math = celestia::math;

namespace
{

constexpr float kRadiusCorrection = 0.025f;
constexpr float kMaxSpiralThickness = 0.06f;

bool
pickBoundingSphere(const DeepSkyObject& dso,
                   const Eigen::ParametrizedLine<double, 3>& ray,
                   double& distanceToPicker,
                   double& cosAngleToBoundCenter)
{
    return dso.isVisible() && math::testIntersection(ray,
                                                     math::Sphered(dso.getPosition(), static_cast<double>(dso.getRadius())),
                                                     distanceToPicker,
                                                     cosAngleToBoundCenter);
}

bool
pickGalaxy(const Galaxy& galaxy,
           const Eigen::ParametrizedLine<double, 3>& ray,
           double& distanceToPicker,
           double& cosAngleToBoundCenter)
{
    const auto* galacticForm = celestia::engine::GalacticFormManager::get()->getForm(galaxy.getFormId());
    if (galacticForm == nullptr || !galaxy.isVisible())
        return false;

    // The ellipsoid should be slightly larger to compensate for the fact
    // that blobs are considered points when galaxies are built, but have size
    // when they are drawn.
    GalaxyType type = galaxy.getGalaxyType();
    float yscale = (type > GalaxyType::Irr && type < GalaxyType::E0)
        ? kMaxSpiralThickness
        : galacticForm->scale.y() + kRadiusCorrection;
    Eigen::Vector3d ellipsoidAxes(galaxy.getRadius()*(galacticForm->scale.x() + kRadiusCorrection),
                                  galaxy.getRadius()* yscale,
                                  galaxy.getRadius()*(galacticForm->scale.z() + kRadiusCorrection));
    Eigen::Matrix3d rotation = galaxy.getOrientation().cast<double>().toRotationMatrix();

    return math::testIntersection(
        math::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - galaxy.getPosition(), ray.direction()),
                           rotation),
        math::Ellipsoidd(ellipsoidAxes),
        distanceToPicker,
        cosAngleToBoundCenter);
}

bool
pickGlobular(const Globular& globular,
             const Eigen::ParametrizedLine<double, 3>& ray,
             double& distanceToPicker,
             double& cosAngleToBoundCenter)
{
    if (!globular.isVisible())
        return false;

    /*
     * The selection sphere should be slightly larger to compensate for the fact
     * that blobs are considered points when globulars are built, but have size
     * when they are drawn.
     */
    Eigen::Vector3d p = globular.getPosition();
    return math::testIntersection(math::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - p, ray.direction()),
                                                     globular.getOrientation().cast<double>().toRotationMatrix()),
                                  math::Sphered(globular.getRadius() * (1.0f + kRadiusCorrection)),
                                  distanceToPicker,
                                  cosAngleToBoundCenter);
}

} // end unnamed namespace

bool
pickDeepSkyObjectGeometry(const DeepSkyObject& dso,
                          const Eigen::ParametrizedLine<double, 3>& ray,
                          double& distanceToPicker,
                          double& cosAngleToBoundCenter)
{
    switch (dso.getObjType())
    {
    case DeepSkyObjectType::Galaxy:
        return pickGalaxy(static_cast<const Galaxy&>(dso), ray, distanceToPicker, cosAngleToBoundCenter);
    case DeepSkyObjectType::Globular:
        return pickGlobular(static_cast<const Globular&>(dso), ray, distanceToPicker, cosAngleToBoundCenter);
    case DeepSkyObjectType::Nebula:
        return pickBoundingSphere(dso, ray, distanceToPicker, cosAngleToBoundCenter);
    case DeepSkyObjectType::OpenCluster:
        return false;
    default:
        return pickBoundingSphere(dso, ray, distanceToPicker, cosAngleToBoundCenter);
    }
}
