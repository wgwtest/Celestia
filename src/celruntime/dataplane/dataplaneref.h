// dataplaneref.h
//
// Copyright (C) 2026, the Celestia Development Team

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace celestia::runtime::dataplane
{

struct DataPlaneRef
{
    std::string kind;
    std::string id;
    std::size_t byteLength{ 0 };
    std::string contentHash;
    std::string transportHint;
};

std::string serializeDataPlaneRef(const DataPlaneRef&);
std::optional<DataPlaneRef> deserializeDataPlaneRef(std::string_view);
std::string dataPlaneContentHash(const std::vector<std::byte>&);

} // namespace celestia::runtime::dataplane
