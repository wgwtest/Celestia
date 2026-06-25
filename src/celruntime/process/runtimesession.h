// runtimesession.h
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

struct RuntimeSessionOptions
{
    std::filesystem::path runtimeHostDirectory;
    std::string viewId{ "celestia.view2d.debug" };
    std::string sessionId{ "default" };
    int durationMilliseconds{ 1000 };
    int tickMilliseconds{ 125 };
    int hostReadyTimeoutMilliseconds{ 3000 };
    int shutdownTimeoutMilliseconds{ 3000 };
    int heartbeatEveryTicks{ 0 };
    std::string controllerTickCommandName{ "controller.tick" };
    int switchViewAfterMilliseconds{ 0 };
    std::string switchViewId;
    std::string hostTransport{ "stdio-pipe" };
};

struct RuntimeSessionResult
{
    bool success{ false };
    bool modelReady{ false };
    bool controllerReady{ false };
    bool viewReady{ false };
    bool modelStopped{ false };
    bool controllerStopped{ false };
    bool viewStopped{ false };
    int modelExitCode{ -1 };
    int controllerExitCode{ -1 };
    int viewExitCode{ -1 };
    int tickCount{ 0 };
    int viewFrameCount{ 0 };
    int viewInputCount{ 0 };
    int heartbeatCount{ 0 };
    bool terminatedHost{ false };
    std::string log;
};

class RuntimeSession
{
public:
    explicit RuntimeSession(RuntimeSessionOptions);

    RuntimeSessionResult run();

private:
    RuntimeSessionOptions options_;
};

} // namespace celestia::runtime::process
