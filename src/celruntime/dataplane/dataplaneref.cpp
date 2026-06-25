// dataplaneref.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "dataplaneref.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace celestia::runtime::dataplane
{
namespace
{

void
appendField(std::string& output, std::string_view key, std::string_view value)
{
    if (!output.empty())
        output.push_back(';');
    output.append(key);
    output.push_back('=');
    output.append(value);
}

std::optional<std::size_t>
parseSize(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoull(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return static_cast<std::size_t>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

} // end unnamed namespace

std::string
serializeDataPlaneRef(const DataPlaneRef& ref)
{
    std::string output;
    appendField(output, "kind", ref.kind);
    appendField(output, "id", ref.id);
    appendField(output, "byteLength", std::to_string(ref.byteLength));
    appendField(output, "contentHash", ref.contentHash);
    appendField(output, "transportHint", ref.transportHint);
    return output;
}

std::optional<DataPlaneRef>
deserializeDataPlaneRef(std::string_view text)
{
    DataPlaneRef ref;
    std::size_t start = 0;
    while (start <= text.size())
    {
        const auto end = text.find(';', start);
        const auto field = text.substr(start, end == std::string_view::npos
                                                 ? std::string_view::npos
                                                 : end - start);
        const auto equals = field.find('=');
        if (equals == std::string_view::npos)
            return std::nullopt;

        const auto key = field.substr(0, equals);
        const auto value = field.substr(equals + 1);
        if (key == "kind")
            ref.kind = std::string(value);
        else if (key == "id")
            ref.id = std::string(value);
        else if (key == "byteLength")
        {
            const auto parsed = parseSize(value);
            if (!parsed.has_value())
                return std::nullopt;
            ref.byteLength = *parsed;
        }
        else if (key == "contentHash")
            ref.contentHash = std::string(value);
        else if (key == "transportHint")
            ref.transportHint = std::string(value);

        if (end == std::string_view::npos)
            break;
        start = end + 1;
    }

    if (ref.kind.empty() || ref.id.empty())
        return std::nullopt;
    return ref;
}

std::string
dataPlaneContentHash(const std::vector<std::byte>& bytes)
{
    std::uint64_t hash = 1469598103934665603ull;
    for (const auto byte : bytes)
    {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 1099511628211ull;
    }

    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << hash;
    return output.str();
}

} // namespace celestia::runtime::dataplane
