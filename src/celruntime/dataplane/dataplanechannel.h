// dataplanechannel.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "dataplaneref.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace celestia::runtime::dataplane
{

class DataPlaneChannel
{
public:
    virtual ~DataPlaneChannel() = default;

    virtual DataPlaneRef publish(const std::vector<std::byte>&, std::string_view label) = 0;
    virtual std::optional<std::vector<std::byte>> acquire(const DataPlaneRef&) const = 0;
    virtual bool release(const DataPlaneRef&) = 0;
};

} // namespace celestia::runtime::dataplane
