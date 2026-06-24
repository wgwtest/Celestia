// starrenderassets.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "starrenderassets.h"

#include <unordered_map>

#include "star.h"
#include "stardetailslifecycle.h"

namespace engine = celestia::engine;
namespace util = celestia::util;

namespace
{

struct StarRenderAssetState
{
    util::TextureHandle texture{ util::TextureHandle::Invalid };
    engine::GeometryHandle geometry{ engine::GeometryHandle::Invalid };
};

StarRenderAssets::TextureSet& textureSet()
{
    static auto* const textures = new StarRenderAssets::TextureSet(); //NOSONAR
    return *textures;
}

std::unordered_map<const StarDetails*, StarRenderAssetState>& assetStates()
{
    static auto* const states = new std::unordered_map<const StarDetails*, StarRenderAssetState>(); //NOSONAR
    return *states;
}

StarRenderAssetState& stateFor(const StarDetails* details)
{
    return assetStates().try_emplace(details).first->second;
}

const StarRenderAssetState* findState(const StarDetails* details)
{
    auto& states = assetStates();
    auto it = states.find(details);
    return it == states.end() ? nullptr : &it->second;
}

util::TextureHandle getTexture(const StarDetails* details)
{
    if (const auto* state = findState(details); state != nullptr)
        return state->texture;

    return util::TextureHandle::Invalid;
}

engine::GeometryHandle getGeometry(const StarDetails* details)
{
    if (const auto* state = findState(details); state != nullptr)
        return state->geometry;

    return engine::GeometryHandle::Invalid;
}

void
ensureStarDetailsLifecycleRegistration()
{
    static const bool registered = []()
    {
        StarDetailsLifecycleEvents::addDestroyedCallback([](const StarDetails* details)
        {
            StarRenderAssets::remove(details);
        });
        StarDetailsLifecycleEvents::addCloneCallback([](const StarDetails* source, const StarDetails* target)
        {
            StarRenderAssets::cloneAssets(source, target);
        });
        StarDetailsLifecycleEvents::addCopyCallback([](const StarDetails* target, const StarDetails* source)
        {
            StarRenderAssets::copyAssets(target, source);
        });
        StarDetailsLifecycleEvents::addCopyTextureIfUnsetCallback([](StarDetails* target, const StarDetails* source)
        {
            StarRenderAssets::copyTextureIfUnset(target, source);
        });
        StarDetailsLifecycleEvents::addNormalStarDefaultCallback([](const StarDetails* details, StellarClass::SpectralClass spectralClass)
        {
            StarRenderAssets::initializeTexture(details, StarRenderAssets::textureFor(spectralClass));
        });
        StarDetailsLifecycleEvents::addWhiteDwarfDefaultCallback([](const StarDetails* details)
        {
            StarRenderAssets::initializeTexture(details, StarRenderAssets::textureFor(StellarClass::Spectral_D));
        });
        StarDetailsLifecycleEvents::addNeutronStarDefaultCallback([](const StarDetails* details)
        {
            StarRenderAssets::initializeTexture(details, StarRenderAssets::neutronStarTexture());
        });
        return true;
    }();
    (void)registered;
}

} // end unnamed namespace

void
StarRenderAssets::setStarTextures(const TextureSet& textures)
{
    ensureStarDetailsLifecycleRegistration();
    textureSet() = textures;
}

util::TextureHandle
StarRenderAssets::getTexture(const Star& star)
{
    ensureStarDetailsLifecycleRegistration();
    return ::getTexture(star.getDetails());
}

engine::GeometryHandle
StarRenderAssets::getGeometry(const Star& star)
{
    ensureStarDetailsLifecycleRegistration();
    return ::getGeometry(star.getDetails());
}

void
StarRenderAssets::setTexture(boost::intrusive_ptr<StarDetails>& details, util::TextureHandle texture)
{
    ensureStarDetailsLifecycleRegistration();
    if (details->shared())
        details = details->clone();

    stateFor(details.get()).texture = texture;
    details->addKnowledge(StarDetails::Knowledge::KnowTexture);
}

void
StarRenderAssets::setGeometry(boost::intrusive_ptr<StarDetails>& details, engine::GeometryHandle geometry)
{
    ensureStarDetailsLifecycleRegistration();
    if (details->shared())
        details = details->clone();

    stateFor(details.get()).geometry = geometry;
}

util::TextureHandle
StarRenderAssets::textureFor(StellarClass::SpectralClass spectralClass)
{
    ensureStarDetailsLifecycleRegistration();
    auto texture = textureSet().starTex[spectralClass];
    return texture == util::TextureHandle::Invalid ? textureSet().defaultTex : texture;
}

util::TextureHandle
StarRenderAssets::neutronStarTexture()
{
    ensureStarDetailsLifecycleRegistration();
    auto texture = textureSet().neutronStarTex;
    return texture == util::TextureHandle::Invalid ? textureSet().defaultTex : texture;
}

void
StarRenderAssets::initializeTexture(const StarDetails* details, util::TextureHandle texture)
{
    ensureStarDetailsLifecycleRegistration();
    stateFor(details).texture = texture;
}

void
StarRenderAssets::cloneAssets(const StarDetails* source, const StarDetails* target)
{
    ensureStarDetailsLifecycleRegistration();
    auto& states = assetStates();
    auto sourceIt = states.find(source);
    if (sourceIt == states.end())
    {
        states.erase(target);
        return;
    }

    states[target] = sourceIt->second;
}

void
StarRenderAssets::copyAssets(const StarDetails* target, const StarDetails* source)
{
    ensureStarDetailsLifecycleRegistration();
    cloneAssets(source, target);
}

void
StarRenderAssets::copyTextureIfUnset(StarDetails* target, const StarDetails* source)
{
    ensureStarDetailsLifecycleRegistration();
    if (target->hasKnowledge(StarDetails::Knowledge::KnowTexture))
        return;

    stateFor(target).texture = ::getTexture(source);
}

void
StarRenderAssets::remove(const StarDetails* details)
{
    ensureStarDetailsLifecycleRegistration();
    assetStates().erase(details);
}
