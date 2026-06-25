// bodyrenderassets.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/adapter/bodyrenderassets.h>

#include <map>
#include <unordered_map>

#include <celengine/model/body.h>
#include <celengine/adapter/bodylifecycle.h>

namespace engine = celestia::engine;
namespace util = celestia::util;

namespace
{

struct BodyRenderAssetState
{
    engine::GeometryHandle geometry{ engine::GeometryHandle::Invalid };
    Eigen::Quaternionf geometryOrientation{ Eigen::Quaternionf::Identity() };
    float geometryScale{ 1.0f };
    Surface surface{ Color(1.0f, 1.0f, 1.0f) };
    std::map<std::string, std::unique_ptr<Surface>, std::less<>> alternateSurfaces;
};

std::unordered_map<const Body*, BodyRenderAssetState>&
assetStates()
{
    static auto* const states = new std::unordered_map<const Body*, BodyRenderAssetState>(); //NOSONAR
    return *states;
}

std::unordered_map<const RingSystem*, util::TextureHandle>&
ringTextures()
{
    static auto* const textures = new std::unordered_map<const RingSystem*, util::TextureHandle>(); //NOSONAR
    return *textures;
}

const BodyRenderAssetState&
defaultState()
{
    static const BodyRenderAssetState state;
    return state;
}

BodyRenderAssetState&
stateFor(const Body* body)
{
    return assetStates().try_emplace(body).first->second;
}

const BodyRenderAssetState&
stateForRead(const Body* body)
{
    auto& states = assetStates();
    auto it = states.find(body);
    return it == states.end() ? defaultState() : it->second;
}

void
ensureBodyLifecycleRegistration()
{
    static const bool registered = []()
    {
        BodyLifecycleEvents::addDestroyedCallback([](const Body* body)
        {
            BodyRenderAssets::remove(body);
        });
        BodyLifecycleEvents::addDefaultPropertiesResetCallback([](const Body* body)
        {
            BodyRenderAssets::reset(body);
        });
        BodyLifecycleEvents::addRingRemovedCallback([](const RingSystem* rings)
        {
            BodyRenderAssets::removeRing(rings);
        });
        BodyLifecycleEvents::addShapeOverrideQuery([](const Body* body)
        {
            return body != nullptr && stateForRead(body).geometry != engine::GeometryHandle::Invalid;
        });
        return true;
    }();
    (void)registered;
}

} // end unnamed namespace

engine::GeometryHandle
BodyRenderAssets::getGeometry(const Body* body)
{
    ensureBodyLifecycleRegistration();
    return stateForRead(body).geometry;
}

void
BodyRenderAssets::setGeometry(const Body* body, engine::GeometryHandle geometry)
{
    ensureBodyLifecycleRegistration();
    stateFor(body).geometry = geometry;
}

Eigen::Quaternionf
BodyRenderAssets::getGeometryOrientation(const Body* body)
{
    ensureBodyLifecycleRegistration();
    return stateForRead(body).geometryOrientation;
}

void
BodyRenderAssets::setGeometryOrientation(const Body* body, const Eigen::Quaternionf& orientation)
{
    ensureBodyLifecycleRegistration();
    stateFor(body).geometryOrientation = orientation;
}

float
BodyRenderAssets::getGeometryScale(const Body* body)
{
    ensureBodyLifecycleRegistration();
    return stateForRead(body).geometryScale;
}

void
BodyRenderAssets::setGeometryScale(const Body* body, float scale)
{
    ensureBodyLifecycleRegistration();
    stateFor(body).geometryScale = scale;
}

const Surface&
BodyRenderAssets::getSurface(const Body* body)
{
    ensureBodyLifecycleRegistration();
    return stateForRead(body).surface;
}

void
BodyRenderAssets::setSurface(const Body* body, const Surface& surface)
{
    ensureBodyLifecycleRegistration();
    stateFor(body).surface = surface;
}

Surface*
BodyRenderAssets::getAlternateSurface(const Body* body, std::string_view name)
{
    ensureBodyLifecycleRegistration();
    const auto& altSurfaces = stateForRead(body).alternateSurfaces;
    auto it = altSurfaces.find(name);
    return it == altSurfaces.end() ? nullptr : it->second.get();
}

void
BodyRenderAssets::setAlternateSurface(const Body* body, std::string_view name, std::unique_ptr<Surface>&& surface)
{
    ensureBodyLifecycleRegistration();
    auto& altSurfaces = stateFor(body).alternateSurfaces;
    if (surface == nullptr)
    {
        auto it = altSurfaces.find(name);
        if (it != altSurfaces.end())
            altSurfaces.erase(it);
        return;
    }

    // C++26 provides additional overloads that allow transparent key updates
    // which would avoid constructing the redundant string when replacing.
    altSurfaces[std::string(name)] = std::move(surface);
}

std::vector<std::string>
BodyRenderAssets::getAlternateSurfaceNames(const Body* body)
{
    ensureBodyLifecycleRegistration();
    std::vector<std::string> names;
    for (const auto& [name, surface] : stateForRead(body).alternateSurfaces)
        names.push_back(name);
    return names;
}

util::TextureHandle
BodyRenderAssets::getRingTexture(const RingSystem* rings)
{
    ensureBodyLifecycleRegistration();
    auto& textures = ringTextures();
    auto it = textures.find(rings);
    return it == textures.end() ? util::TextureHandle::Invalid : it->second;
}

void
BodyRenderAssets::setRingTexture(const RingSystem* rings, util::TextureHandle texture)
{
    ensureBodyLifecycleRegistration();
    if (rings == nullptr)
        return;

    ringTextures()[rings] = texture;
}

void
BodyRenderAssets::removeRing(const RingSystem* rings)
{
    ensureBodyLifecycleRegistration();
    ringTextures().erase(rings);
}

void
BodyRenderAssets::reset(const Body* body)
{
    ensureBodyLifecycleRegistration();
    assetStates().erase(body);
}

void
BodyRenderAssets::remove(const Body* body)
{
    ensureBodyLifecycleRegistration();
    assetStates().erase(body);
}
