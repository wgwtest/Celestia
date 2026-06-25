// modelservice.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>

#include "modelsnapshot.h"

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::model
{

class ModelService
{
public:
    explicit ModelService(std::string sessionId = "default");
    ModelService(std::string sessionId, std::unique_ptr<SimulationBackend> backend);
    ~ModelService();

    ModelService(ModelService&&) noexcept;
    ModelService& operator=(ModelService&&) noexcept;

    ModelService(const ModelService&) = delete;
    ModelService& operator=(const ModelService&) = delete;

    bool isRunning() const;
    bool isPaused() const;
    double simulationTime() const;

    protocol::RuntimeEnvelope handle(const protocol::RuntimeEnvelope&);

private:
    protocol::RuntimeEnvelope lifecycleResponse(const protocol::RuntimeEnvelope& request,
                                                std::string name) const;
    protocol::RuntimeEnvelope errorResponse(const protocol::RuntimeEnvelope& request,
                                            std::string message) const;
    protocol::RuntimeEnvelope viewFrameResponse(const protocol::RuntimeEnvelope& request) const;

    std::string sessionId_;
    bool running_{ false };
    bool paused_{ false };
    double timeScale_{ 1.0 };
    std::unique_ptr<SimulationBackend> backend_;
};

} // namespace celestia::runtime::model
