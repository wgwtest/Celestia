// solarsystem.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <unordered_map>

#include <celengine/model/astroobj.h>

class FrameTree;
class PlanetarySystem;
class Star;

class SolarSystem
{
public:
    SolarSystem(Star*);
    ~SolarSystem();

    Star* getStar() const;
    PlanetarySystem* getPlanets() const;
    FrameTree* getFrameTree() const;

private:
    Star* star;
    std::unique_ptr<PlanetarySystem> planets;
    std::unique_ptr<FrameTree> frameTree;
};

using SolarSystemCatalog = std::unordered_map<AstroCatalog::IndexNumber, std::unique_ptr<SolarSystem>>;
