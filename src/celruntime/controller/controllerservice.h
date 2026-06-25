// controllerservice.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::controller
{

class ControllerService
{
public:
    explicit ControllerService(std::string sessionId = "default");

    bool isRunning() const;

    std::vector<protocol::RuntimeEnvelope> handle(const protocol::RuntimeEnvelope&);

private:
    protocol::RuntimeEnvelope commandToModel(const protocol::RuntimeEnvelope& request,
                                             std::string name,
                                             std::string payload = {}) const;
    protocol::RuntimeEnvelope lifecycleToLauncher(const protocol::RuntimeEnvelope& request,
                                                  std::string name) const;
    protocol::RuntimeEnvelope errorResponse(const protocol::RuntimeEnvelope& request,
                                            std::string message) const;

    std::string sessionId_;
    bool running_{ false };
    bool paused_{ false };
};

} // namespace celestia::runtime::controller
