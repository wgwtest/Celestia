// texmanager.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <unordered_map>

#include <celengine/resource/texturepaths.h>
#include <celengine/view3d/texture.h>

namespace celestia::engine
{

class TextureManager
{
public:
    TextureManager(std::shared_ptr<const TexturePaths>, TextureResolution);

    Texture* find(util::TextureHandle handle);
    Texture* findShadow(util::TextureHandle handle);
    TextureResolution resolution() const noexcept { return m_resolution; }
    void resolution(TextureResolution);

private:
    std::shared_ptr<const TexturePaths> m_paths;
    std::unordered_map<util::TextureHandle, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<util::TextureHandle, std::shared_ptr<Texture>> m_shadowTextures;
    TextureResolution m_resolution;
};

} // end namespace celestia::engine
