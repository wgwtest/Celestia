// viewproviderregistry.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "viewproviderregistry.h"

#include <utility>

namespace celestia::runtime
{

bool
ViewProviderRegistry::empty() const
{
    return m_providers.empty();
}

std::size_t
ViewProviderRegistry::size() const
{
    return m_providers.size();
}

bool
ViewProviderRegistry::registerProvider(ViewProvider provider)
{
    if (provider.id.empty() || !provider.create)
        return false;

    const auto id = provider.id;
    return m_providers.emplace(id, std::move(provider)).second;
}

const ViewProvider*
ViewProviderRegistry::find(std::string_view id) const
{
    const auto iter = m_providers.find(std::string(id));
    return iter == m_providers.end() ? nullptr : &iter->second;
}

std::unique_ptr<ViewRuntime>
ViewProviderRegistry::create(std::string_view id) const
{
    const auto* provider = find(id);
    if (provider == nullptr || !provider->create)
        return nullptr;

    return provider->create();
}

} // namespace celestia::runtime
