// starrenderassets.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <celutil/texhandle.h>
#include <celengine/view3d/meshmanager.h>
#include <celengine/model/stellarclass.h>

class Star;
class StarDetails;

class StarRenderAssets
{
public:
    struct TextureSet
    {
        TextureSet() { starTex.fill(celestia::util::TextureHandle::Invalid); }

        celestia::util::TextureHandle defaultTex{ celestia::util::TextureHandle::Invalid };
        celestia::util::TextureHandle neutronStarTex{ celestia::util::TextureHandle::Invalid };
        std::array<celestia::util::TextureHandle, StellarClass::Spectral_Count> starTex;
    };

    static void setStarTextures(const TextureSet&);

    static celestia::util::TextureHandle getTexture(const Star&);
    static celestia::engine::GeometryHandle getGeometry(const Star&);

    static void setTexture(boost::intrusive_ptr<StarDetails>&, celestia::util::TextureHandle);
    static void setGeometry(boost::intrusive_ptr<StarDetails>&, celestia::engine::GeometryHandle);

    static celestia::util::TextureHandle textureFor(StellarClass::SpectralClass);
    static celestia::util::TextureHandle neutronStarTexture();

    static void initializeTexture(const StarDetails*, celestia::util::TextureHandle);
    static void cloneAssets(const StarDetails*, const StarDetails*);
    static void copyAssets(const StarDetails*, const StarDetails*);
    static void copyTextureIfUnset(StarDetails*, const StarDetails*);
    static void remove(const StarDetails*);
};
