// runtimeconfig.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>

namespace celestia::runtime
{

class RuntimeConfig
{
public:
    static constexpr std::string_view DefaultViewId{ "celestia.view3d.opengl" };

    RuntimeConfig();

    const std::string& selectedViewId() const;
    void setSelectedViewId(std::string);

private:
    std::string m_selectedViewId;
};

} // namespace celestia::runtime
