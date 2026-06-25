// debug2dprovider.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "debug2dprovider.h"

#include <memory>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/viewframe.h>

namespace celestia::viewproviders
{
namespace
{

constexpr std::string_view Debug2DViewProviderId{ "celestia.view2d.debug" };

const char*
boolText(bool value)
{
    return value ? "true" : "false";
}

std::string
formatTime(double time)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << time;

    auto text = output.str();
    while (text.size() > 1 && text.back() == '0')
        text.pop_back();
    if (!text.empty() && text.back() == '.')
        text.pop_back();

    return text;
}

std::string
formatFrameSummary(const celestia::runtime::ViewFrame& frame)
{
    std::ostringstream output;
    output << "time=" << formatTime(frame.time)
           << " selections=" << frame.selections.size();

    if (!frame.selections.empty())
    {
        const auto& selection = frame.selections.front();
        output << " selected=" << selection.id
               << " type=" << selection.type
               << " visible=" << boolText(selection.visible)
               << " clickable=" << boolText(selection.clickable);
    }

    return output.str();
}

class Debug2DViewRuntime final : public celestia::runtime::ViewRuntime
{
public:
    Debug2DViewRuntime() = default;
    ~Debug2DViewRuntime() override = default;

    std::string_view id() const override
    {
        return Debug2DViewProviderId;
    }

    void present(const celestia::runtime::ViewFrame& frame) override
    {
        m_lastFrameSummary = formatFrameSummary(frame);
    }

    std::string_view lastFrameSummary() const override
    {
        return m_lastFrameSummary;
    }

private:
    std::string m_lastFrameSummary;
};

} // end unnamed namespace

celestia::runtime::ViewProvider
makeDebug2DViewProvider()
{
    celestia::runtime::ViewProvider provider;
    provider.id = std::string(Debug2DViewProviderId);
    provider.displayName = "Celestia Debug 2D View";
    provider.dimension = "2d";
    provider.create = []()
    {
        return std::make_unique<Debug2DViewRuntime>();
    };
    return provider;
}

} // namespace celestia::viewproviders
