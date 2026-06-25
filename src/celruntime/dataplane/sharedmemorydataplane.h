// sharedmemorydataplane.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include "dataplanechannel.h"

#include <memory>

namespace celestia::runtime::dataplane
{

class SharedMemoryDataPlane : public DataPlaneChannel
{
public:
    SharedMemoryDataPlane();
    ~SharedMemoryDataPlane() override;

    SharedMemoryDataPlane(const SharedMemoryDataPlane&) = delete;
    SharedMemoryDataPlane& operator=(const SharedMemoryDataPlane&) = delete;

    DataPlaneRef publish(const std::vector<std::byte>&, std::string_view label) override;
    std::optional<std::vector<std::byte>> acquire(const DataPlaneRef&) const override;
    bool release(const DataPlaneRef&) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace celestia::runtime::dataplane
