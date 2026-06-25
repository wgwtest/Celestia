// transportfactory.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "runtimetransport.h"

#include <memory>
#include <string>
#include <string_view>

#include <celruntime/process/hostprocess.h>

namespace celestia::runtime::transport
{

bool isSupportedRuntimeTransport(std::string_view kind);

std::unique_ptr<RuntimeTransport> createRuntimeTransport(
    std::string_view kind,
    process::HostProcessOptions options,
    std::string* error = nullptr);

} // namespace celestia::runtime::transport
