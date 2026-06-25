// inprocessdataplane.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "inprocessdataplane.h"

#include <utility>

namespace celestia::runtime::dataplane
{

DataPlaneRef
InProcessDataPlane::publish(const std::vector<std::byte>& bytes, std::string_view label)
{
    auto id = std::string(label) + "-" + std::to_string(nextId_++);
    blocks_[id] = bytes;

    DataPlaneRef ref;
    ref.kind = "in-process";
    ref.id = std::move(id);
    ref.byteLength = bytes.size();
    ref.contentHash = dataPlaneContentHash(bytes);
    ref.transportHint = "framed-envelope";
    return ref;
}

std::optional<std::vector<std::byte>>
InProcessDataPlane::acquire(const DataPlaneRef& ref) const
{
    if (ref.kind != "in-process")
        return std::nullopt;

    const auto iter = blocks_.find(ref.id);
    if (iter == blocks_.end())
        return std::nullopt;

    if (iter->second.size() != ref.byteLength ||
        dataPlaneContentHash(iter->second) != ref.contentHash)
    {
        return std::nullopt;
    }

    return iter->second;
}

bool
InProcessDataPlane::release(const DataPlaneRef& ref)
{
    return blocks_.erase(ref.id) > 0;
}

} // namespace celestia::runtime::dataplane
