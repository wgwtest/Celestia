// texmanager.cpp
//
// Copyright (C) 2001-present, Celestia Development Team
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/view3d/texmanager.h>

#include <celutil/logger.h>
#include <celengine/view3d/texture.h>

using celestia::util::GetLogger;

namespace celestia::engine
{
namespace
{
std::unique_ptr<Texture>
LoadTexture(const TextureInfo& info)
{
    Texture::AddressMode addressMode = Texture::EdgeClamp;
    Texture::MipMapMode  mipMode     = Texture::DefaultMipMaps;
    Texture::Colorspace  colorspace  = Texture::DefaultColorspace;

    if (util::is_set(info.flags, TextureFlags::WrapTexture))
        addressMode = Texture::Wrap;
    else if (util::is_set(info.flags, TextureFlags::BorderClamp))
        addressMode = Texture::BorderClamp;

    if (util::is_set(info.flags, TextureFlags::NoMipMaps))
        mipMode = Texture::NoMipMaps;

    if (util::is_set(info.flags, TextureFlags::LinearColorspace))
        colorspace = Texture::LinearColorspace;

    if (info.bumpHeight == 0.0f)
    {
        GetLogger()->debug("Loading texture: {}\n", info.path);
        return LoadTextureFromFile(info.path, addressMode, mipMode, colorspace);
    }

    GetLogger()->debug("Loading bump map: {}\n", info.path);
    return LoadHeightMapFromFile(info.path, info.bumpHeight, addressMode);
}

} // end unnamed namespace

TextureManager::TextureManager(std::shared_ptr<const TexturePaths> paths,
                               TextureResolution resolution) :
    m_paths(paths),
    m_resolution(resolution)
{
}

Texture*
TextureManager::find(util::TextureHandle handle)
{
    if (handle == util::TextureHandle::Invalid)
        return nullptr;

    auto [it, inserted] = m_textures.try_emplace(handle);
    if (!inserted)
        return it->second.get();

    GetLogger()->info("Loading texture handle {}\n", static_cast<std::uint32_t>(handle));

    TextureInfo info;
    if (!m_paths->getInfo(handle, m_resolution, info))
        return nullptr;

    it->second = LoadTexture(info);
    return it->second.get();
}

Texture*
TextureManager::findShadow(util::TextureHandle handle)
{
    // shadow textures will only load the medres texture
    constexpr auto ShadowResolution = TextureResolution::medres;

    if (handle == util::TextureHandle::Invalid)
        return nullptr;

    auto [it, inserted] = m_shadowTextures.try_emplace(handle);
    if (!inserted)
        return it->second.get();

    if (m_resolution <= ShadowResolution ||
        m_paths->samePath(handle, m_resolution, ShadowResolution))
    {
        auto [mainIt, mainInserted] = m_textures.try_emplace(handle);
        if (!mainInserted)
        {
            it->second = mainIt->second;
            return it->second.get();
        }

        TextureInfo info;
        if (!m_paths->getInfo(handle, m_resolution, info))
            return nullptr;

        it->second = mainIt->second = LoadTexture(info);
        return it->second.get();
    }

    TextureInfo info;
    if (!m_paths->getInfo(handle, ShadowResolution, info))
        return nullptr;

    it->second = LoadTexture(info);
    return it->second.get();
}

void
TextureManager::resolution(TextureResolution resolution)
{
    if (resolution == m_resolution)
        return;

    m_textures.clear();
    m_shadowTextures.clear();
    m_resolution = resolution;
}

} // end namespace celestia::engine
