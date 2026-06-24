// universe.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// A container for catalogs of galaxies, stars, and planets.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "universe.h"

#include <algorithm>
#include <utility>

#include <celutil/greek.h>
#include <celutil/utf8.h>
#include "asterism.h"
#include "body.h"
#include "boundaries.h"
#include "location.h"
#include "urlmanager.h"

namespace engine = celestia::engine;
namespace util = celestia::util;

namespace
{

class ClosestStarFinder : public engine::StarHandler
{
public:
    ClosestStarFinder(float _maxDistance, const Universe* _universe);
    ~ClosestStarFinder() = default;
    void process(const Star& star, float distance, float appMag) override;

public:
    float maxDistance;
    float closestDistance;
    const Star* closestStar;
    const Universe* universe;
    bool withPlanets;
};

ClosestStarFinder::ClosestStarFinder(float _maxDistance,
                                     const Universe* _universe) :
    maxDistance(_maxDistance),
    closestDistance(_maxDistance),
    closestStar(nullptr),
    universe(_universe),
    withPlanets(false)
{
}

void
ClosestStarFinder::process(const Star& star, float distance, float /*unused*/)
{
    if (distance < closestDistance)
    {
        if (!withPlanets || universe->getSolarSystem(&star))
        {
            closestStar = &star;
            closestDistance = distance;
        }
    }
}

class NearStarFinder : public engine::StarHandler
{
public:
    NearStarFinder(float _maxDistance, std::vector<const Star*>& nearStars);
    ~NearStarFinder() = default;
    void process(const Star& star, float distance, float appMag);

private:
    float maxDistance;
    std::vector<const Star*>& nearStars;
};

NearStarFinder::NearStarFinder(float _maxDistance,
                               std::vector<const Star*>& _nearStars) :
    maxDistance(_maxDistance),
    nearStars(_nearStars)
{
}

void
NearStarFinder::process(const Star& star, float distance, float /*unused*/)
{
    if (distance < maxDistance)
        nearStars.push_back(&star);
}

void
getLocationsCompletion(std::vector<celestia::engine::Completion>& completion,
                       std::string_view s,
                       const Body& body)
{
    auto locations = GetBodyFeaturesManager()->getLocations(&body);
    if (!locations.has_value())
        return;

    for (const auto location : *locations)
    {
        const std::string& name = location->getName(false);
        if (UTF8StartsWith(name, s))
        {
            completion.emplace_back(name, Selection(location));
        }
        else
        {
            const std::string& lname = location->getName(true);
            if (lname != name && UTF8StartsWith(lname, s))
                completion.emplace_back(lname, Selection(location));
        }
    }
}

} // end unnamed namespace

Universe::Universe(std::unique_ptr<engine::UrlManager>&& _urlManager) :
    urlManager(std::move(_urlManager))
{
}

// Needs definition of ConstellationBoundaries
Universe::~Universe() = default;

StarDatabase*
Universe::getStarCatalog() const
{
    return starCatalog.get();
}

void
Universe::setStarCatalog(std::unique_ptr<StarDatabase>&& catalog)
{
    starCatalog = std::move(catalog);
}

SolarSystemCatalog*
Universe::getSolarSystemCatalog() const
{
    return solarSystemCatalog.get();
}

void
Universe::setSolarSystemCatalog(std::unique_ptr<SolarSystemCatalog>&& catalog)
{
    solarSystemCatalog = std::move(catalog);
}

DSODatabase*
Universe::getDSOCatalog() const
{
    return dsoCatalog.get();
}

void
Universe::setDSOCatalog(std::unique_ptr<DSODatabase>&& catalog)
{
    dsoCatalog = std::move(catalog);
}

AsterismList*
Universe::getAsterisms() const
{
    return asterisms.get();
}

void
Universe::setAsterisms(std::unique_ptr<AsterismList>&& _asterisms)
{
    asterisms = std::move(_asterisms);
}

ConstellationBoundaries*
Universe::getBoundaries() const
{
    return boundaries.get();
}

void
Universe::setBoundaries(std::unique_ptr<ConstellationBoundaries>&& _boundaries)
{
    boundaries = std::move(_boundaries);
}

// Return the planetary system of a star, or nullptr if it has no planets.
SolarSystem*
Universe::getSolarSystem(const Star* star) const
{
    if (star == nullptr)
        return nullptr;

    auto starNum = star->getIndex();
    auto iter = solarSystemCatalog->find(starNum);
    return iter == solarSystemCatalog->end()
        ? nullptr
        : iter->second.get();
}

// A more general version of the method above--return the solar system
// that contains an object, or nullptr if there is no solar sytstem.
SolarSystem*
Universe::getSolarSystem(const Selection& sel) const
{
    switch (sel.getType())
    {
    case SelectionType::Star:
        return getSolarSystem(sel.star());

    case SelectionType::Body:
        {
            PlanetarySystem* system = sel.body()->getSystem();
            while (system != nullptr)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != nullptr)
                    system = parent->getSystem();
                else
                    return getSolarSystem(Selection(system->getStar()));
            }
            return nullptr;
        }

    case SelectionType::Location:
        return getSolarSystem(Selection(sel.location()->getParentBody()));

    default:
        return nullptr;
    }
}

const celestia::MarkerList&
Universe::getMarkers() const
{
    return markers;
}

void
Universe::markObject(const Selection& sel,
                     const celestia::MarkerRepresentation& rep,
                     int priority,
                     bool occludable,
                     celestia::MarkerSizing sizing)
{
    if (auto iter = std::find_if(markers.begin(), markers.end(),
                                 [&sel](const auto& m) { return m.object() == sel; });
        iter != markers.end())
    {
        // Handle the case when the object is already marked.  If the
        // priority is higher or equal to the existing marker, replace it.
        // Otherwise, do nothing.
        if (priority < iter->priority())
            return;
        markers.erase(iter);
    }

    celestia::Marker& marker = markers.emplace_back(sel);
    marker.setRepresentation(rep);
    marker.setPriority(priority);
    marker.setOccludable(occludable);
    marker.setSizing(sizing);
}

void
Universe::unmarkObject(const Selection& sel, int priority)
{
    auto iter = std::find_if(markers.begin(), markers.end(),
                             [&sel](const auto& m) { return m.object() == sel; });
    if (iter != markers.end() && priority >= iter->priority())
        markers.erase(iter);
}

void
Universe::unmarkAll()
{
    markers.clear();
}

bool
Universe::isMarked(const Selection& sel, int priority) const
{
    auto iter = std::find_if(markers.begin(), markers.end(),
                             [&sel](const auto& m) { return m.object() == sel; });
    return iter != markers.end() && iter->priority() >= priority;
}

// Search by name for an immediate child of the specified object.
Selection
Universe::findChildObject(const Selection& sel,
                          std::string_view name,
                          bool i18n) const
{
    switch (sel.getType())
    {
    case SelectionType::Star:
        if (const SolarSystem* sys = getSolarSystem(sel.star()); sys != nullptr)
        {
            PlanetarySystem* planets = sys->getPlanets();
            if (planets != nullptr)
                return Selection(planets->find(name, false, i18n));
        }
        break;

    case SelectionType::Body:
        // First, search for a satellite
        if (const PlanetarySystem* sats = sel.body()->getSatellites();sats != nullptr)
        {
            Body* body = sats->find(name, false, i18n);
            if (body != nullptr)
                return Selection(body);
        }

        // If a satellite wasn't found, check this object's locations
        if (Location* loc = GetBodyFeaturesManager()->findLocation(sel.body(), name, i18n);
            loc != nullptr)
        {
            return Selection(loc);
        }
        break;

    default:
        // Locations and deep sky objects have no children
        break;
    }

    return Selection();
}

// Search for a name within an object's context.  For stars, planets (bodies),
// and locations, the context includes all bodies in the associated solar
// system.  For locations and planets, the context additionally includes
// sibling or child locations, respectively.
Selection
Universe::findObjectInContext(const Selection& sel,
                              std::string_view name,
                              bool i18n) const
{
    const Body* contextBody = nullptr;

    switch (sel.getType())
    {
    case SelectionType::Body:
        contextBody = sel.body();
        break;

    case SelectionType::Location:
        contextBody = sel.location()->getParentBody();
        break;

    default:
        break;
    }

    // First, search for bodies...
    if (const SolarSystem* sys = getSolarSystem(sel); sys != nullptr)
    {
        if (const PlanetarySystem* planets = sys->getPlanets(); planets != nullptr)
        {
            if (Body* body = planets->find(name, true, i18n); body != nullptr)
                return Selection(body);
        }
    }

    // ...and then locations.
    if (contextBody != nullptr)
    {
        Location* loc = GetBodyFeaturesManager()->findLocation(contextBody, name, i18n);
        if (loc != nullptr)
            return Selection(loc);
    }

    return Selection();
}

// Select an object by name, with the following priority:
//   1. Try to look up the name in the star catalog
//   2. Search the deep sky catalog for a matching name.
//   3. Check the solar systems for planet names; we don't make any decisions
//      about which solar systems are relevant, and let the caller pass them
//      to us to search.
Selection
Universe::find(std::string_view s,
               util::array_view<Selection> contexts,
               bool i18n) const
{
    if (starCatalog != nullptr)
    {
        Star* star = starCatalog->find(s, i18n);
        if (star != nullptr)
            return Selection(star);
        star = starCatalog->find(ReplaceGreekLetterAbbr(s), i18n);
        if (star != nullptr)
            return Selection(star);
    }

    if (dsoCatalog != nullptr)
    {
        DeepSkyObject* dso = dsoCatalog->find(s, i18n);
        if (dso != nullptr)
            return Selection(dso);
        dso = dsoCatalog->find(ReplaceGreekLetterAbbr(s), i18n);
        if (dso != nullptr)
            return Selection(dso);
    }

    for (const auto& context : contexts)
    {
        Selection sel = findObjectInContext(context, s, i18n);
        if (!sel.empty())
            return sel;
    }

    return Selection();
}

// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.  The caller may pass in a list of solar systems
// to search for objects--this is roughly analgous to the PATH environment
// variable in Unix and Windows.  Typically, the solar system will be one
// in which the user is currently located.
Selection
Universe::findPath(std::string_view s,
                   util::array_view<Selection> contexts,
                   bool i18n) const
{
    std::string_view::size_type pos = s.find('/', 0);

    // No delimiter found--just do a normal find.
    if (pos == std::string_view::npos)
        return find(s, contexts, i18n);

    // Find the base object
    auto base = s.substr(0, pos);
    Selection sel = find(base, contexts, i18n);

    while (!sel.empty() && pos != std::string_view::npos)
    {
        auto nextPos = s.find('/', pos + 1);
        auto len = nextPos == std::string_view::npos
                 ? s.size() - pos - 1
                 : nextPos - pos - 1;

        auto name = s.substr(pos + 1, len);

        sel = findChildObject(sel, name, i18n);

        pos = nextPos;
    }

    return sel;
}

void
Universe::getCompletion(std::vector<celestia::engine::Completion>& completion,
                        std::string_view s,
                        util::array_view<Selection> contexts,
                        bool withLocations) const
{
    // Solar bodies first:
    for (const Selection& context : contexts)
    {
        if (withLocations && context.getType() == SelectionType::Body)
        {
            getLocationsCompletion(completion, s, *context.body());
        }

        const SolarSystem* sys = getSolarSystem(context);
        if (sys != nullptr)
        {
            const PlanetarySystem* planets = sys->getPlanets();
            if (planets != nullptr)
                planets->getCompletion(completion, s);
        }
    }

    // Deep sky objects:
    if (dsoCatalog != nullptr)
        dsoCatalog->getCompletion(completion, s);

    // and finally stars;
    if (starCatalog != nullptr)
        starCatalog->getCompletion(completion, s);
}

void
Universe::getCompletionPath(std::vector<celestia::engine::Completion>& completion,
                            std::string_view s,
                            util::array_view<Selection> contexts,
                            bool withLocations) const
{
    std::string_view::size_type pos = s.rfind('/', s.length());

    if (pos == std::string_view::npos)
    {
        getCompletion(completion, s, contexts, withLocations);
        return;
    }

    auto base = s.substr(0, pos);
    Selection sel = findPath(base, contexts, true);

    if (sel.empty())
    {
        return;
    }

    if (sel.getType() == SelectionType::DeepSky)
    {
        completion.emplace_back(dsoCatalog->getDSOName(sel.deepsky()), sel);
        return;
    }

    const PlanetarySystem* worlds = nullptr;
    if (sel.getType() == SelectionType::Body)
    {
        worlds = sel.body()->getSatellites();
    }
    else if (sel.getType() == SelectionType::Star)
    {
        const SolarSystem* ssys = getSolarSystem(sel.star());
        if (ssys != nullptr)
            worlds = ssys->getPlanets();
    }

    if (worlds != nullptr)
        worlds->getCompletion(completion, s.substr(pos + 1), false);

    if (sel.getType() == SelectionType::Body && withLocations)
    {
        getLocationsCompletion(completion,
                               s.substr(pos + 1),
                               *sel.body());
    }
}

// Return the closest solar system to position, or nullptr if there are no planets
// with in one light year.
SolarSystem*
Universe::getNearestSolarSystem(const UniversalCoord& position) const
{
    Eigen::Vector3f pos = position.toLy().cast<float>();
    ClosestStarFinder closestFinder(1.0f, this);
    closestFinder.withPlanets = true;
    starCatalog->findCloseStars(closestFinder, pos, 1.0f);
    return getSolarSystem(closestFinder.closestStar);
}

void
Universe::getNearStars(const UniversalCoord& position,
                       float maxDistance,
                       std::vector<const Star*>& nearStars) const
{
    Eigen::Vector3f pos = position.toLy().cast<float>();
    NearStarFinder finder(maxDistance, nearStars);
    starCatalog->findCloseStars(finder, pos, maxDistance);
}

std::string_view
Universe::getInfoURL(const Selection& selection) const
{
    return urlManager->getURL(selection);
}
