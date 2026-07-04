// solarsystem.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/model/solarsystem.h>

#include <memory>

#include <celengine/model/body.h>
#include <celengine/model/frametree.h>

SolarSystem::SolarSystem(Star* _star) :
    star(_star)
{
    planets = std::make_unique<PlanetarySystem>(star);
    frameTree = std::make_unique<FrameTree>(star);
}

SolarSystem::~SolarSystem() = default;

Star*
SolarSystem::getStar() const
{
    return star;
}

PlanetarySystem*
SolarSystem::getPlanets() const
{
    return planets.get();
}

FrameTree*
SolarSystem::getFrameTree() const
{
    return frameTree.get();
}
