// viewproviderregistry.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "viewprovider.h"

namespace celestia::runtime
{

class ViewProviderRegistry
{
public:
    bool empty() const;
    std::size_t size() const;

    bool registerProvider(ViewProvider);
    const ViewProvider* find(std::string_view) const;
    std::unique_ptr<ViewRuntime> create(std::string_view) const;

private:
    std::map<std::string, ViewProvider> m_providers;
};

} // namespace celestia::runtime
