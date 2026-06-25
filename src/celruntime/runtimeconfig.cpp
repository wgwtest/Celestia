// runtimeconfig.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimeconfig.h"

#include <utility>

namespace celestia::runtime
{

RuntimeConfig::RuntimeConfig() :
    m_selectedViewId(DefaultViewId)
{
}

const std::string&
RuntimeConfig::selectedViewId() const
{
    return m_selectedViewId;
}

void
RuntimeConfig::setSelectedViewId(std::string viewId)
{
    m_selectedViewId = std::move(viewId);
}

} // namespace celestia::runtime
