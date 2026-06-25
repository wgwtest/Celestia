// transportfactory.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "transportfactory.h"

#include "localsockettransport.h"
#include "stdiopipetransport.h"

#include <utility>

namespace celestia::runtime::transport
{
namespace
{

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

} // end unnamed namespace

bool
isSupportedRuntimeTransport(std::string_view kind)
{
    return kind == "stdio" || kind == "stdio-pipe" || kind == "local-socket";
}

std::unique_ptr<RuntimeTransport>
createRuntimeTransport(std::string_view kind,
                       process::HostProcessOptions options,
                       std::string* error)
{
    if (kind == "stdio" || kind == "stdio-pipe")
        return std::make_unique<StdioPipeTransport>(std::move(options));

    if (kind == "local-socket")
    {
        if (!localSocketTransportSupported())
        {
            setError(error, "unsupported transport: local-socket");
            return nullptr;
        }
        return std::make_unique<LocalSocketTransport>(std::move(options));
    }

    setError(error, "unsupported transport: " + std::string(kind));
    return nullptr;
}

} // namespace celestia::runtime::transport
