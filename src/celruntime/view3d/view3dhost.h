// view3dhost.h
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
#include <vector>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::view3d
{

class View3DHost
{
public:
    explicit View3DHost(std::string sessionId = "default");

    bool isRunning() const;
    std::uint64_t frameCount() const;
    std::uint64_t lastSequence() const;
    double lastSimulationTime() const;

    std::vector<protocol::RuntimeEnvelope> handle(const protocol::RuntimeEnvelope&);

private:
    protocol::RuntimeEnvelope response(const protocol::RuntimeEnvelope& request,
                                       protocol::RuntimeMessageKind kind,
                                       std::string name,
                                       std::string payload = {}) const;
    protocol::RuntimeEnvelope errorResponse(const protocol::RuntimeEnvelope& request,
                                            std::string message) const;
    protocol::RuntimeEnvelope ready3D(const protocol::RuntimeEnvelope& request) const;
    protocol::RuntimeEnvelope frameRendered(const protocol::RuntimeEnvelope& request) const;

    std::string sessionId_;
    bool running_{ false };
    std::uint64_t frameCount_{ 0 };
    std::uint64_t lastSequence_{ 0 };
    double lastSimulationTime_{ 0.0 };
    std::uint64_t lastBodyCount_{ 0 };
    std::uint64_t lastStarCount_{ 0 };
};

} // namespace celestia::runtime::view3d
