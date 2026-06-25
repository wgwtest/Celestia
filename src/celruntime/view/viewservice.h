// viewservice.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::view
{

class ViewService
{
public:
    explicit ViewService(std::string sessionId = "default");

    bool isRunning() const;
    std::uint64_t lastFrameId() const;
    double lastSimulationTime() const;
    std::string_view lastFrameSummary() const;
    std::uint64_t frameCount() const;

    std::vector<protocol::RuntimeEnvelope> handle(const protocol::RuntimeEnvelope&);
    protocol::RuntimeEnvelope closeRequested() const;

private:
    protocol::RuntimeEnvelope lifecycleResponse(const protocol::RuntimeEnvelope& request,
                                                std::string name) const;
    protocol::RuntimeEnvelope errorResponse(const protocol::RuntimeEnvelope& request,
                                            std::string message) const;

    std::string sessionId_;
    bool running_{ false };
    std::uint64_t lastFrameId_{ 0 };
    double lastSimulationTime_{ 0.0 };
    std::string lastFrameSummary_;
    std::uint64_t frameCount_{ 0 };
};

} // namespace celestia::runtime::view
