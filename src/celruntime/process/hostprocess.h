// hostprocess.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <celruntime/protocol/envelope.h>

namespace celestia::runtime::process
{

struct HostProcessOptions
{
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path workingDirectory;
};

class HostProcess
{
public:
    explicit HostProcess(HostProcessOptions);
    ~HostProcess();

    HostProcess(const HostProcess&) = delete;
    HostProcess& operator=(const HostProcess&) = delete;

    bool start();
    bool send(const protocol::RuntimeEnvelope&);
    std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds timeout);
    std::optional<int> wait(std::chrono::milliseconds timeout);
    bool terminate();
    bool running() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace celestia::runtime::process
