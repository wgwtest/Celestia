// view3dprovider.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dprovider.h"

#include <memory>
#include <string>
#include <string_view>

#include <celengine/view3d/render.h>

namespace celestia::viewproviders
{
namespace
{

constexpr std::string_view OpenGLViewProviderId{ "celestia.view3d.opengl" };

class OpenGLViewRuntime final : public celestia::runtime::ViewRuntime
{
public:
    OpenGLViewRuntime() = default;
    ~OpenGLViewRuntime() override = default;

    std::string_view id() const override
    {
        return OpenGLViewProviderId;
    }

private:
    std::unique_ptr<Renderer> m_renderer;
};

} // end unnamed namespace

celestia::runtime::ViewProvider
makeOpenGLViewProvider()
{
    celestia::runtime::ViewProvider provider;
    provider.id = std::string(OpenGLViewProviderId);
    provider.displayName = "Celestia OpenGL 3D View";
    provider.dimension = "3d";
    provider.create = []()
    {
        return std::make_unique<OpenGLViewRuntime>();
    };
    return provider;
}

} // namespace celestia::viewproviders
