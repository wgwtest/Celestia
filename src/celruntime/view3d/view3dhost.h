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
#include <filesystem>
#include <string>
#include <vector>

#include <celruntime/protocol/envelope.h>
#include <celruntime/view3d/view3dscene.h>

namespace celestia::runtime::view3d
{

struct View3DHostOptions
{
    std::filesystem::path contentRoot;
};

class View3DHost
{
public:
    explicit View3DHost(std::string sessionId = "default",
                        View3DHostOptions options = {});

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
    View3DHostOptions options_;
    View3DSceneState lastSceneState_;
};

} // namespace celestia::runtime::view3d
