// selectionpicker.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/selectionpicker.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include <celcompat/numbers.h>
#include <celephem/orbit.h>
#include <celmath/intersect.h>
#include <celmath/mathlib.h>
#include <celmath/ray.h>
#include <celutil/flag.h>
#include <celengine/model/body.h>
#include <celengine/model/deepskyobj.h>
#include <celengine/adapter/deepskyobjectpicker.h>
#include <celengine/adapter/deepskyobjectrenderpolicy.h>
#include <celengine/model/frametree.h>
#include <celengine/adapter/selectiongeometryprovider.h>
#include <celengine/adapter/solarsys.h>
#include <celengine/model/stardb.h>
#include <celengine/model/timelinephase.h>
#include <celengine/model/universe.h>

namespace engine = celestia::engine;
namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

constexpr double ANGULAR_RES = 3.5e-6;

struct PlanetPickInfo
{
    double sinAngle2Closest;
    double closestDistance;
    double closestApproxDistance;
    Body* closestBody;
    Eigen::ParametrizedLine<double, 3> pickRay;
    double jd;
    float atanTolerance;
};

bool
ApproxPlanetPickTraversal(Body* body, PlanetPickInfo& pickInfo)
{
    // Reject invisible bodies and bodies that don't exist at the current time
    if (!body->isVisible() || !body->extant(pickInfo.jd) || !body->isClickable())
        return true;

    Eigen::Vector3d bpos = body->getAstrocentricPosition(pickInfo.jd);
    Eigen::Vector3d bodyDir = bpos - pickInfo.pickRay.origin();
    double distance = bodyDir.norm();

    // Check the apparent radius of the orbit against our tolerance factor.
    // This check exists to make sure than when picking a distant, we select
    // the planet rather than one of its satellites.
    if (auto appOrbitRadius = static_cast<float>(body->getOrbit(pickInfo.jd)->getBoundingRadius() / distance);
        std::max(static_cast<double>(pickInfo.atanTolerance), ANGULAR_RES) > appOrbitRadius)
    {
        return true;
    }

    bodyDir.normalize();
    Eigen::Vector3d bodyMiss = bodyDir - pickInfo.pickRay.direction();
    if (double sinAngle2 = bodyMiss.norm() / 2.0; sinAngle2 <= pickInfo.sinAngle2Closest)
    {
        pickInfo.sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickInfo.closestBody = body;
        pickInfo.closestApproxDistance = distance;
    }

    return true;
}

class ExactPlanetPickTraversal
{
public:
    explicit ExactPlanetPickTraversal(const SelectionGeometryProvider& provider) : geometryProvider(provider) {}

    bool operator()(Body* body, PlanetPickInfo& pickInfo) const;

private:
    const SelectionGeometryProvider& geometryProvider;
};

// Perform an intersection test between the pick ray and a body
bool
ExactPlanetPickTraversal::operator()(Body* body,
                                     PlanetPickInfo& pickInfo) const
{
    Eigen::Vector3d bpos = body->getAstrocentricPosition(pickInfo.jd);
    float radius = body->getRadius();
    double distance = -1.0;

    // Test for intersection with the bounding sphere
    if (!body->isVisible() || !body->extant(pickInfo.jd) || !body->isClickable() ||
        !math::testIntersection(pickInfo.pickRay, math::Sphered(bpos, radius), distance))
        return true;

    auto geometryHandle = geometryProvider.geometryFor(body);
    if (geometryHandle == engine::GeometryHandle::Invalid)
    {
        // There's no mesh, so the object is an ellipsoid.  If it's
        // spherical, we've already done all the work we need to. Otherwise,
        // we need to perform a ray-ellipsoid intersection test.
        if (!body->isSphere())
        {
            Eigen::Vector3d ellipsoidAxes = body->getSemiAxes().cast<double>();

            // Transform rotate the pick ray into object coordinates
            Eigen::Matrix3d m = body->getEclipticToEquatorial(pickInfo.jd).toRotationMatrix();
            Eigen::ParametrizedLine<double, 3> r(pickInfo.pickRay.origin() - bpos, pickInfo.pickRay.direction());
            r = math::transformRay(r, m);
            if (!math::testIntersection(r, math::Ellipsoidd(ellipsoidAxes), distance))
                distance = -1.0;
        }
    }
    else
    {
        // Transform rotate the pick ray into object coordinates
        Eigen::Quaterniond qd = geometryProvider.geometryOrientationFor(body).cast<double>();
        Eigen::Matrix3d m = (qd * body->getEclipticToBodyFixed(pickInfo.jd)).toRotationMatrix();
        Eigen::ParametrizedLine<double, 3> r(pickInfo.pickRay.origin() - bpos, pickInfo.pickRay.direction());
        r = math::transformRay(r, m);

        const Geometry* geometry = geometryProvider.findGeometry(geometryHandle);
        float scaleFactor = geometryProvider.geometryScaleFor(body);
        if (geometry != nullptr && geometry->isNormalized())
            scaleFactor = radius;

        // The mesh vertices are normalized, then multiplied by a scale
        // factor.  Thus, the ray needs to be multiplied by the inverse of
        // the mesh scale factor.
        double is = 1.0 / scaleFactor;
        r.origin() *= is;
        r.direction() *= is;

        if (geometry != nullptr && !geometry->pick(r, distance))
            distance = -1.0;
    }
    // Make also sure that the pickRay does not intersect the body in the
    // opposite hemisphere! Hence, need again the "bodyMiss" angle

    Eigen::Vector3d bodyDir = bpos - pickInfo.pickRay.origin();
    bodyDir.normalize();
    Eigen::Vector3d bodyMiss = bodyDir - pickInfo.pickRay.direction();

    if (double sinAngle2 = bodyMiss.norm() / 2.0;
        sinAngle2 < (celestia::numbers::sqrt2 * 0.5) && // sin(45 degrees) = sqrt(2)/2
        distance > 0.0 && distance <= pickInfo.closestDistance)
    {
        pickInfo.closestDistance = distance;
        pickInfo.closestBody = body;
    }

    return true;
}

// Recursively traverse a frame tree; call the specified callback function for each
// body in the tree. The callback function returns a boolean indicating whether
// traversal should continue.
//
// TODO: This function works, but could use some cleanup:
//   * Make it a member of the frame tree class
//   * Combine info and func into a traversal callback class
template<typename F>
bool
traverseFrameTree(const FrameTree* frameTree,
                  double tdb,
                  F func,
                  PlanetPickInfo& info)
{
    for (unsigned int i = 0; i < frameTree->childCount(); i++)
    {
        const TimelinePhase* phase = frameTree->getChild(i);
        if (!phase->includes(tdb))
            continue;

        Body* body = phase->body();
        if (!func(body, info))
            return false;

        if (const FrameTree* bodyFrameTree = body->getFrameTree();
            bodyFrameTree != nullptr && !traverseFrameTree(bodyFrameTree, tdb, func, info))
        {
            return false;
        }
    }

    return true;
}

// StarPicker is a callback class for StarDatabase::findVisibleStars
class StarPicker : public engine::StarHandler
{
public:
    StarPicker(const Eigen::Vector3f&, const Eigen::Vector3f&, double, float);
    ~StarPicker() = default;

    void process(const Star& /*star*/, float /*unused*/, float /*unused*/) override;

public:
    const Star* pickedStar;
    Eigen::Vector3f pickOrigin;
    Eigen::Vector3f pickRay;
    double sinAngle2Closest;
    double when;
};

StarPicker::StarPicker(const Eigen::Vector3f& _pickOrigin,
                       const Eigen::Vector3f& _pickRay,
                       double _when,
                       float angle) :
    pickedStar(nullptr),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    sinAngle2Closest(std::max(std::sin(angle / 2.0), ANGULAR_RES)),
    when(_when)
{
}

void
StarPicker::process(const Star& star, float /*unused*/, float /*unused*/)
{
    Eigen::Vector3f relativeStarPos = star.getPosition() - pickOrigin;
    Eigen::Vector3f starDir = relativeStarPos.normalized();

    double sinAngle2 = 0.0;

    // Stars with orbits need special handling
    float orbitalRadius = star.getOrbitalRadius();
    if (orbitalRadius != 0.0f)
    {
        float distance = 0.0f;

        // Check for an intersection with orbital bounding sphere; if there's
        // no intersection, then just use normal calculation.  We actually test
        // intersection with a larger sphere to make sure we don't miss a star
        // right on the edge of the sphere.
        if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), pickRay),
                                   math::Spheref(relativeStarPos, orbitalRadius * 2.0f),
                                   distance))
        {
            Eigen::Vector3d starPos = star.getPosition(when).toLy();
            starDir = (starPos - pickOrigin.cast<double>()).cast<float>().normalized();
        }
    }

    Eigen::Vector3f starMiss = starDir - pickRay;
    Eigen::Vector3d sMd = starMiss.cast<double>();
    sinAngle2 = sMd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedStar = &star;
        if (pickedStar->getOrbitBarycenter() != nullptr)
            pickedStar = pickedStar->getOrbitBarycenter();
    }
}

class CloseStarPicker : public engine::StarHandler
{
public:
    CloseStarPicker(const UniversalCoord& pos,
                    const Eigen::Vector3f& dir,
                    double t,
                    float _maxDistance,
                    float angle);
    ~CloseStarPicker() = default;
    void process(const Star& star, float lowPrecDistance, float appMag) override;

public:
    UniversalCoord pickOrigin;
    Eigen::Vector3f pickDir;
    double now;
    float maxDistance;
    const Star* closestStar;
    float closestDistance;
    double sinAngle2Closest;
};

CloseStarPicker::CloseStarPicker(const UniversalCoord& pos,
                                 const Eigen::Vector3f& dir,
                                 double t,
                                 float _maxDistance,
                                 float angle) :
    pickOrigin(pos),
    pickDir(dir),
    now(t),
    maxDistance(_maxDistance),
    closestStar(nullptr),
    closestDistance(0.0f),
    sinAngle2Closest(std::max(std::sin(angle/2.0), ANGULAR_RES))
{
}

void
CloseStarPicker::process(const Star& star,
                         float lowPrecDistance,
                         float /*unused*/)
{
    if (lowPrecDistance > maxDistance)
        return;

    Eigen::Vector3d hPos = star.getPosition(now).offsetFromKm(pickOrigin);
    Eigen::Vector3f starDir = hPos.cast<float>();

    float distance = 0.0f;

     if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), pickDir),
                                math::Spheref(starDir, star.getRadius()), distance))
    {
        if (distance > 0.0f)
        {
            if (closestStar == nullptr || distance < closestDistance)
            {
                closestStar = &star;
                closestDistance = starDir.norm();
                sinAngle2Closest = ANGULAR_RES;
                // An exact hit--set the angle to "zero"
            }
        }
    }
    else
    {
        // We don't have an exact hit; check to see if we're close enough
        float distance = starDir.norm();
        starDir.normalize();
        Eigen::Vector3f starMiss = starDir - pickDir;
        Eigen::Vector3d sMd = starMiss.cast<double>();

        double sinAngle2 = sMd.norm() / 2.0;

        if (sinAngle2 <= sinAngle2Closest &&
            (closestStar == nullptr || distance < closestDistance))
        {
            closestStar = &star;
            closestDistance = distance;
            sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        }
    }
}

class DSOPicker : public engine::DSOHandler
{
public:
    DSOPicker(const Eigen::Vector3d& pickOrigin,
              const Eigen::Vector3d& pickDir,
              RenderFlags renderFlags,
              float angle);
    ~DSOPicker() = default;

    void process(const std::unique_ptr<DeepSkyObject>&, double, float) override; //NOSONAR

public:
    Eigen::Vector3d pickOrigin;
    Eigen::Vector3d pickDir;
    RenderFlags renderFlags;

    const DeepSkyObject* pickedDSO;
    double  sinAngle2Closest;
};

DSOPicker::DSOPicker(const Eigen::Vector3d& pickOrigin,
                     const Eigen::Vector3d& pickDir,
                     RenderFlags renderFlags,
                     float angle) :
    pickOrigin      (pickOrigin),
    pickDir         (pickDir),
    renderFlags     (renderFlags),
    pickedDSO       (nullptr),
    sinAngle2Closest(std::max(std::sin(angle / 2.0), ANGULAR_RES))
{
}

void
DSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso, double, float) //NOSONAR
{
    if (!util::is_set(renderFlags, getDeepSkyObjectRenderMask(*dso)) || !dso->isVisible() || !dso->isClickable())
        return;

    Eigen::Vector3d relativeDSOPos = dso->getPosition() - pickOrigin;
    Eigen::Vector3d dsoDir = relativeDSOPos;

    if (double distance2 = 0.0;
        math::testIntersection(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), pickDir),
                               math::Sphered(relativeDSOPos, (double) dso->getRadius()), distance2))
    {
        Eigen::Vector3d dsoPos = dso->getPosition();
        dsoDir = dsoPos * 1.0e-6 - pickOrigin;
    }
    dsoDir.normalize();

    Eigen::Vector3d dsoMissd   = dsoDir - pickDir;
    double sinAngle2 = dsoMissd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedDSO        = dso.get();
    }
}

class CloseDSOPicker : public engine::DSOHandler
{
public:
    CloseDSOPicker(const Eigen::Vector3d& pos,
                   const Eigen::Vector3d& dir,
                   RenderFlags renderFlags,
                   double maxDistance,
                   float);
    ~CloseDSOPicker() = default;

    void process(const std::unique_ptr<DeepSkyObject>& dso, double distance, float appMag); //NOSONAR

public:
    Eigen::Vector3d  pickOrigin;
    Eigen::Vector3d  pickDir;
    RenderFlags renderFlags;
    double    maxDistance;

    const DeepSkyObject* closestDSO;
    double largestCosAngle;
};

CloseDSOPicker::CloseDSOPicker(const Eigen::Vector3d& pos,
                               const Eigen::Vector3d& dir,
                               RenderFlags renderFlags,
                               double maxDistance,
                               float /*unused*/) :
    pickOrigin     (pos),
    pickDir        (dir),
    renderFlags    (renderFlags),
    maxDistance    (maxDistance),
    closestDSO     (nullptr),
    largestCosAngle(-2.0)
{
}

void
CloseDSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso, //NOSONAR
                        double distance,
                        float /*unused*/)
{
    if (distance > maxDistance || !util::is_set(renderFlags, getDeepSkyObjectRenderMask(*dso)) || !dso->isVisible() || !dso->isClickable())
        return;

    double  distanceToPicker       = 0.0;
    double  cosAngleToBoundCenter  = 0.0;
    if (pickDeepSkyObjectGeometry(*dso,
                                  Eigen::ParametrizedLine<double, 3>(pickOrigin, pickDir),
                                  distanceToPicker,
                                  cosAngleToBoundCenter))
    {
        // Don't select the object the observer is currently in:
        if ((pickOrigin - dso->getPosition()).norm() > dso->getRadius() &&
            cosAngleToBoundCenter > largestCosAngle)
        {
            closestDSO      = dso.get();
            largestCosAngle = cosAngleToBoundCenter;
        }
    }
}

} // end unnamed namespace

SelectionPicker::SelectionPicker(const Universe& _universe,
                                 const SelectionGeometryProvider& _geometryProvider) :
    universe(_universe),
    geometryProvider(_geometryProvider)
{
}

Selection
SelectionPicker::pickPlanet(const SolarSystem& solarSystem,
                            const UniversalCoord& origin,
                            const Eigen::Vector3f& direction,
                            double when,
                            float /*faintestMag*/,
                            float tolerance) const
{
    double sinTol2 = std::max(std::sin(tolerance / 2.0), ANGULAR_RES);
    PlanetPickInfo pickInfo;

    Star* star = solarSystem.getStar();
    assert(star != nullptr);

    // Transform the pick ray origin into astrocentric coordinates
    Eigen::Vector3d astrocentricOrigin = origin.offsetFromKm(star->getPosition(when));

    pickInfo.pickRay = Eigen::ParametrizedLine<double, 3>(astrocentricOrigin, direction.cast<double>());
    pickInfo.sinAngle2Closest = 1.0;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestApproxDistance = 1.0e50;
    pickInfo.closestBody = nullptr;
    pickInfo.jd = when;
    pickInfo.atanTolerance = (float) atan(tolerance);

    // First see if there's a planet|moon that the pick ray intersects.
    // Select the closest planet|moon intersected.
    traverseFrameTree(solarSystem.getFrameTree(), when, ExactPlanetPickTraversal(geometryProvider), pickInfo);

    if (pickInfo.closestBody != nullptr)
    {
        // Retain that body
        Body* closestBody = pickInfo.closestBody;

        // Check if there is a satellite in front of the primary body that is
        // sufficiently close to the pickRay
        traverseFrameTree(solarSystem.getFrameTree(), when, &ApproxPlanetPickTraversal, pickInfo);

        if (pickInfo.closestBody == closestBody)
            return  Selection(closestBody);
        // Nothing else around, select the body and return

        // Are we close enough to the satellite and is it in front of the body?
        if ((pickInfo.sinAngle2Closest <= sinTol2) &&
            (pickInfo.closestDistance > pickInfo.closestApproxDistance))
            return Selection(pickInfo.closestBody);
            // Yes, select the satellite
        else
            return  Selection(closestBody);
           //  No, select the primary body
    }

    // If no planet was intersected by the pick ray, choose the planet|moon
    // with the smallest angular separation from the pick ray.  Very distant
    // planets are likley to fail the intersection test even if the user
    // clicks on a pixel where the planet's disc has been rendered--in order
    // to make distant planets visible on the screen at all, their apparent
    // size has to be greater than their actual disc size.
    traverseFrameTree(solarSystem.getFrameTree(), when, &ApproxPlanetPickTraversal, pickInfo);

    if (pickInfo.sinAngle2Closest <= sinTol2)
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}

Selection
SelectionPicker::pickStar(const UniversalCoord& origin,
                          const Eigen::Vector3f& direction,
                          double when,
                          float faintestMag,
                          float tolerance) const
{
    Eigen::Vector3f o = origin.toLy().cast<float>();
    StarDatabase* starCatalog = universe.getStarCatalog();

    // Use a high precision pick test for any stars that are close to the
    // observer.  If this test fails, use a low precision pick test for stars
    // which are further away.  All this work is necessary because the low
    // precision pick test isn't reliable close to a star and the high
    // precision test isn't nearly fast enough to use on our database of
    // over 100k stars.
    CloseStarPicker closePicker(origin, direction, when, 1.0f, tolerance);
    starCatalog->findCloseStars(closePicker, o, 1.0f);
    if (closePicker.closestStar != nullptr)
        return Selection(const_cast<Star*>(closePicker.closestStar));

    // Find visible stars expects an orientation, but we just have a direction
    // vector.  Convert the direction vector into an orientation by computing
    // the rotation required to map -Z to the direction.
    Eigen::Quaternionf rotation;
    rotation.setFromTwoVectors(-Eigen::Vector3f::UnitZ(), direction);

    StarPicker picker(o, direction, when, tolerance);
    starCatalog->findVisibleStars(picker,
                                  o,
                                  rotation.conjugate(),
                                  tolerance, 1.0f,
                                  faintestMag);
    if (picker.pickedStar != nullptr)
        return Selection(const_cast<Star*>(picker.pickedStar));
    else
        return Selection();
}

Selection
SelectionPicker::pickDeepSkyObject(const UniversalCoord& origin,
                                   const Eigen::Vector3f& direction,
                                   RenderFlags renderFlags,
                                   float faintestMag,
                                   float tolerance) const
{
    Eigen::Vector3d orig = origin.toLy();
    Eigen::Vector3d dir = direction.cast<double>();
    DSODatabase* dsoCatalog = universe.getDSOCatalog();

    CloseDSOPicker closePicker(orig, dir, renderFlags, 1e9, tolerance);

    dsoCatalog->findCloseDSOs(closePicker, orig, 1e9);
    if (closePicker.closestDSO != nullptr)
    {
        return Selection(const_cast<DeepSkyObject*>(closePicker.closestDSO));
    }

    Eigen::Quaternionf rotation;
    rotation.setFromTwoVectors(-Eigen::Vector3f::UnitZ(), direction);

    DSOPicker picker(orig, dir, renderFlags, tolerance);
    dsoCatalog->findVisibleDSOs(picker,
                                orig,
                                rotation.conjugate(),
                                tolerance,
                                1.0f,
                                faintestMag);
    if (picker.pickedDSO != nullptr)
        return Selection(const_cast<DeepSkyObject*>(picker.pickedDSO));
    else
        return Selection();
}

Selection
SelectionPicker::pick(const UniversalCoord& origin,
                      const Eigen::Vector3f& direction,
                      double when,
                      RenderFlags renderFlags,
                      float faintestMag,
                      float tolerance) const
{
    Selection sel;

    if (util::is_set(renderFlags, RenderFlags::ShowPlanets))
    {
        std::vector<const Star*> closeStars;
        universe.getNearStars(origin, 1.0f, closeStars);
        for (const auto star : closeStars)
        {
            const SolarSystem* solarSystem = universe.getSolarSystem(star);
            if (solarSystem != nullptr)
            {
                sel = pickPlanet(*solarSystem,
                                 origin, direction,
                                 when,
                                 faintestMag,
                                 tolerance);
                if (!sel.empty())
                    break;
            }
        }
    }

    if (sel.empty() && util::is_set(renderFlags, RenderFlags::ShowStars))
    {
        sel = pickStar(origin, direction, when, faintestMag, tolerance);
    }

    if (sel.empty())
    {
        sel = pickDeepSkyObject(origin, direction, renderFlags, faintestMag, tolerance);
    }

    return sel;
}
