// inprocessdataplane.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "dataplanechannel.h"

#include <cstdint>
#include <map>
#include <string>

namespace celestia::runtime::dataplane
{

class InProcessDataPlane : public DataPlaneChannel
{
public:
    DataPlaneRef publish(const std::vector<std::byte>&, std::string_view label) override;
    std::optional<std::vector<std::byte>> acquire(const DataPlaneRef&) const override;
    bool release(const DataPlaneRef&) override;

private:
    std::map<std::string, std::vector<std::byte>> blocks_;
    std::uint64_t nextId_{ 1 };
};

} // namespace celestia::runtime::dataplane
