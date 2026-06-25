// runtimehostcommon.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "runtimehostcommon.h"

#include "runtimehostloop.h"
#include "view3dhostloop.h"

#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <celruntime/ipc/message.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/transport/localsockettransport.h>

namespace celestia::runtime::process
{
namespace
{

struct RuntimeHostOptions
{
    bool stdio{ false };
    std::string localSocketEndpoint;
    bool once{ false };
    bool serve{ false };
    int protocolVersion{ ipc::CurrentProtocolVersion };
    int heartbeatMilliseconds{ 1000 };
    std::string sessionId;
    std::string viewId{ RuntimeConfig::DefaultViewId };
};

bool
startsWith(std::string_view text, std::string_view prefix)
{
    return text.substr(0, prefix.size()) == prefix;
}

std::optional<int>
parseInt(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoi(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<RuntimeHostOptions>
parseOptions(int argc, char* argv[], std::string& error)
{
    RuntimeHostOptions options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view argument{ argv[i] };
        if (argument == "--stdio")
        {
            options.stdio = true;
        }
        else if (startsWith(argument, "--local-socket="))
        {
            options.localSocketEndpoint = std::string(argument.substr(15));
        }
        else if (argument == "--once")
        {
            options.once = true;
        }
        else if (argument == "--serve")
        {
            options.serve = true;
        }
        else if (startsWith(argument, "--protocol-version="))
        {
            const auto value = parseInt(argument.substr(19));
            if (!value.has_value())
            {
                error = "invalid protocol version";
                return std::nullopt;
            }
            options.protocolVersion = *value;
        }
        else if (startsWith(argument, "--view="))
        {
            options.viewId = std::string(argument.substr(7));
        }
        else if (startsWith(argument, "--session="))
        {
            options.sessionId = std::string(argument.substr(10));
        }
        else if (startsWith(argument, "--heartbeat-ms="))
        {
            const auto value = parseInt(argument.substr(15));
            if (!value.has_value() || *value <= 0)
            {
                error = "invalid heartbeat interval";
                return std::nullopt;
            }
            options.heartbeatMilliseconds = *value;
        }
        else
        {
            error = "unknown argument: " + std::string(argument);
            return std::nullopt;
        }
    }

    return options;
}

int
fail(std::ostream& error, std::string_view message)
{
    error << message << '\n';
    return 2;
}

std::string
readAll(std::istream& input)
{
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // end unnamed namespace

int
runRuntimeHost(std::string_view role,
               int argc,
               char* argv[],
               std::istream& input,
               std::ostream& output,
               std::ostream& error)
{
    std::string optionError;
    auto options = parseOptions(argc, argv, optionError);
    if (!options.has_value())
        return fail(error, optionError);

    const auto localSocket = !options->localSocketEndpoint.empty();
    if (options->stdio == localSocket)
        return fail(error, "exactly one host transport is required");

    if (options->once == options->serve)
        return fail(error, "exactly one of --once or --serve is required");

    if (options->protocolVersion != ipc::CurrentProtocolVersion)
        return fail(error, "unsupported protocol version");

    if (options->serve)
    {
        std::unique_ptr<transport::FramedTransport> localTransport;
        if (localSocket)
        {
            std::string transportError;
            localTransport = transport::connectLocalSocketEndpoint(options->localSocketEndpoint,
                                                                   &transportError);
            if (localTransport == nullptr)
                return fail(error, transportError);
        }

        if (role == "view3d")
        {
            auto sessionId = options->sessionId;
            if (sessionId.empty())
                sessionId = "default";
            if (localTransport != nullptr)
                return runRuntimeView3DHostLoop(std::move(sessionId), *localTransport, error);
            return runRuntimeView3DHostLoop(std::move(sessionId), input, output, error);
        }

        const auto roleValue = runtimeRoleFromHostRole(role);
        if (!roleValue.has_value())
            return fail(error, "unknown runtime host role");

        auto sessionId = options->sessionId;
        if (sessionId.empty())
            sessionId = "default";
        if (localTransport != nullptr)
            return runRuntimeHostLoop(*roleValue, std::move(sessionId), *localTransport, error);
        return runRuntimeHostLoop(*roleValue, std::move(sessionId), input, output, error);
    }

    const auto requestText = readAll(input);
    const auto request = ipc::deserializeMessage(requestText);
    if (!request.has_value())
        return fail(error, "invalid handshake message");

    if (request->kind != ipc::MessageKind::Command || request->name != "runtime.handshake")
        return fail(error, "expected runtime.handshake command");

    output << ipc::serializeMessage(
        ipc::RuntimeMessage::event("runtime.ack",
                                   "role=" + std::string(role) + ";view=" + options->viewId));
    return 0;
}

} // namespace celestia::runtime::process
