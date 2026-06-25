// viewpluginmanifest.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "viewpluginmanifest.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace celestia::runtime::viewplugin
{
namespace
{

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

std::optional<std::size_t>
findValueStart(std::string_view json, std::string_view key)
{
    const auto quotedKey = "\"" + std::string(key) + "\"";
    const auto keyOffset = json.find(quotedKey);
    if (keyOffset == std::string_view::npos)
        return std::nullopt;

    const auto colon = json.find(':', keyOffset + quotedKey.size());
    if (colon == std::string_view::npos)
        return std::nullopt;

    auto offset = colon + 1;
    while (offset < json.size() && std::isspace(static_cast<unsigned char>(json[offset])) != 0)
        ++offset;
    return offset;
}

std::optional<std::string>
parseStringAt(std::string_view json, std::size_t offset)
{
    if (offset >= json.size() || json[offset] != '"')
        return std::nullopt;

    std::string output;
    for (auto i = offset + 1; i < json.size(); ++i)
    {
        const auto ch = json[i];
        if (ch == '"')
            return output;
        if (ch == '\\' && i + 1 < json.size())
        {
            output.push_back(json[++i]);
            continue;
        }
        output.push_back(ch);
    }

    return std::nullopt;
}

std::optional<std::size_t>
findStringEnd(std::string_view json, std::size_t offset)
{
    if (offset >= json.size() || json[offset] != '"')
        return std::nullopt;

    for (auto i = offset + 1; i < json.size(); ++i)
    {
        if (json[i] == '\\' && i + 1 < json.size())
        {
            ++i;
            continue;
        }
        if (json[i] == '"')
            return i;
    }

    return std::nullopt;
}

std::string
stringField(std::string_view json, std::string_view key)
{
    const auto offset = findValueStart(json, key);
    if (!offset.has_value())
        return {};

    return parseStringAt(json, *offset).value_or(std::string{});
}

int
intField(std::string_view json, std::string_view key)
{
    const auto offset = findValueStart(json, key);
    if (!offset.has_value())
        return 0;

    try
    {
        std::size_t consumed = 0;
        return std::stoi(std::string(json.substr(*offset)), &consumed);
    }
    catch (...)
    {
        return 0;
    }
}

std::vector<std::string>
stringArrayField(std::string_view json, std::string_view key)
{
    std::vector<std::string> values;
    const auto offset = findValueStart(json, key);
    if (!offset.has_value() || *offset >= json.size() || json[*offset] != '[')
        return values;

    const auto arrayEnd = json.find(']', *offset + 1);
    if (arrayEnd == std::string_view::npos)
        return values;

    auto cursor = *offset + 1;
    while (cursor < arrayEnd)
    {
        const auto valueStart = json.find('"', cursor);
        if (valueStart == std::string_view::npos || valueStart >= arrayEnd)
            break;

        auto value = parseStringAt(json, valueStart);
        if (!value.has_value())
            break;
        values.push_back(std::move(*value));

        const auto valueEnd = findStringEnd(json, valueStart);
        if (!valueEnd.has_value())
            break;
        cursor = *valueEnd + 1;
    }

    return values;
}

bool
missingRequired(const ViewPluginManifest& manifest, std::string& field)
{
    if (manifest.id.empty())
        field = "id";
    else if (manifest.name.empty())
        field = "name";
    else if (manifest.kind.empty())
        field = "kind";
    else if (manifest.version.empty())
        field = "version";
    else if (manifest.entry.empty())
        field = "entry";
    else
        return false;

    return true;
}

} // end unnamed namespace

bool
ViewPluginManifest::supports(std::string_view capability) const
{
    return std::find(capabilities.begin(), capabilities.end(), std::string(capability)) != capabilities.end();
}

std::optional<ViewPluginManifest>
parseViewPluginManifest(std::string_view text, std::string* error)
{
    ViewPluginManifest manifest;
    manifest.id = stringField(text, "id");
    manifest.name = stringField(text, "name");
    manifest.kind = stringField(text, "kind");
    manifest.version = stringField(text, "version");
    manifest.protocolVersion = intField(text, "protocolVersion");
    manifest.entry = stringField(text, "entry");
    manifest.processMode = stringField(text, "processMode");
    manifest.capabilities = stringArrayField(text, "capabilities");
    manifest.supportedTransports = stringArrayField(text, "supportedTransports");
    manifest.resourceRequirements = stringArrayField(text, "resourceRequirements");

    if (manifest.processMode.empty())
        manifest.processMode = "host-process";

    std::string missing;
    if (missingRequired(manifest, missing))
    {
        setError(error, "missing required manifest field: " + missing);
        return std::nullopt;
    }

    if (manifest.protocolVersion != CurrentViewPluginProtocolVersion)
    {
        setError(error, "unsupported manifest protocolVersion");
        return std::nullopt;
    }

    return manifest;
}

std::optional<ViewPluginManifest>
loadViewPluginManifest(const std::filesystem::path& path, std::string* error)
{
    std::ifstream input(path);
    if (!input.good())
    {
        setError(error, "failed to open manifest: " + path.string());
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parseViewPluginManifest(buffer.str(), error);
}

} // namespace celestia::runtime::viewplugin
