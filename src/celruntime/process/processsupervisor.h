// processsupervisor.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <string>

namespace celestia::runtime::process
{

struct ProcessSupervisorOptions
{
    std::filesystem::path runtimeHostDirectory;
    std::string viewId{ "celestia.view2d.debug" };
    int durationMilliseconds{ 500 };
    std::string hostTransport{ "stdio" };
    std::string sessionId{ "default" };
};

struct ProcessSupervisorResult
{
    bool success{ false };
    bool modelReady{ false };
    bool controllerReady{ false };
    bool viewReady{ false };
    int modelExitCode{ -1 };
    int controllerExitCode{ -1 };
    int viewExitCode{ -1 };
    int viewFrameCount{ 0 };
    std::string log;
};

class ProcessSupervisor
{
public:
    explicit ProcessSupervisor(ProcessSupervisorOptions);

    ProcessSupervisorResult runServeSmoke() const;

private:
    ProcessSupervisorOptions options_;
};

} // namespace celestia::runtime::process
