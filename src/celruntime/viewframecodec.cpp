// viewframecodec.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "viewframecodec.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace celestia::runtime
{
namespace
{

constexpr std::array<char, 16> HexDigits{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

bool
isUnreserved(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

std::string
escape(std::string_view text)
{
    std::string output;
    for (const auto c : text)
    {
        if (isUnreserved(c))
        {
            output.push_back(c);
            continue;
        }

        const auto value = static_cast<unsigned char>(c);
        output.push_back('%');
        output.push_back(HexDigits[(value >> 4U) & 0x0FU]);
        output.push_back(HexDigits[value & 0x0FU]);
    }

    return output;
}

int
hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

std::optional<std::string>
unescape(std::string_view text)
{
    std::string output;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] != '%')
        {
            output.push_back(text[i]);
            continue;
        }

        if (i + 2 >= text.size())
            return std::nullopt;

        const auto high = hexValue(text[i + 1]);
        const auto low = hexValue(text[i + 2]);
        if (high < 0 || low < 0)
            return std::nullopt;

        output.push_back(static_cast<char>((high << 4U) | low));
        i += 2;
    }

    return output;
}

std::unordered_map<std::string, std::string>
parsePayload(std::string_view payload)
{
    std::unordered_map<std::string, std::string> fields;
    std::size_t offset = 0;

    while (offset <= payload.size())
    {
        auto end = payload.find(';', offset);
        if (end == std::string_view::npos)
            end = payload.size();

        const auto part = payload.substr(offset, end - offset);
        const auto separator = part.find('=');
        if (separator != std::string_view::npos)
        {
            const auto value = unescape(part.substr(separator + 1));
            if (value.has_value())
                fields[std::string(part.substr(0, separator))] = *value;
        }

        if (end == payload.size())
            break;
        offset = end + 1;
    }

    return fields;
}

std::optional<double>
parseDouble(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stod(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::uint64_t>
parseUint64(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoull(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return static_cast<std::uint64_t>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool
parseBool(std::string_view text)
{
    return text == "true" || text == "1" || text == "yes";
}

std::string
formatDouble(double value)
{
    std::ostringstream output;
    output << std::setprecision(17) << value;
    return output.str();
}

void
appendField(std::ostringstream& output, std::string_view key, std::string_view value)
{
    output << ';' << key << '=' << escape(value);
}

void
appendField(std::ostringstream& output, std::string_view key, double value)
{
    appendField(output, key, formatDouble(value));
}

void
appendField(std::ostringstream& output, std::string_view key, bool value)
{
    appendField(output, key, value ? std::string_view{ "true" } : std::string_view{ "false" });
}

void
appendField(std::ostringstream& output, std::string_view key, std::uint64_t value)
{
    appendField(output, key, std::to_string(value));
}

template<std::size_t N>
void
appendArray(std::ostringstream& output, std::string_view prefix, const std::array<double, N>& values)
{
    for (std::size_t i = 0; i < N; ++i)
        appendField(output, std::string(prefix) + std::to_string(i), values[i]);
}

std::string
getField(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    return iter == fields.end() ? std::string{} : iter->second;
}

std::optional<double>
getDouble(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    return iter == fields.end() ? std::nullopt : parseDouble(iter->second);
}

std::uint64_t
getUint64(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    if (iter == fields.end())
        return 0;

    return parseUint64(iter->second).value_or(0);
}

bool
getBool(const std::unordered_map<std::string, std::string>& fields, std::string_view key, bool fallback = false)
{
    const auto iter = fields.find(std::string(key));
    return iter == fields.end() ? fallback : parseBool(iter->second);
}

template<std::size_t N>
std::array<double, N>
getArray(const std::unordered_map<std::string, std::string>& fields,
         std::string_view prefix,
         const std::array<double, N>& fallback)
{
    auto result = fallback;
    for (std::size_t i = 0; i < N; ++i)
    {
        if (const auto value = getDouble(fields, std::string(prefix) + std::to_string(i)); value.has_value())
            result[i] = *value;
    }

    return result;
}

void
appendResource(std::ostringstream& output, std::string_view prefix, const ViewFrameResource& resource)
{
    appendField(output, std::string(prefix) + ".id", resource.id);
    appendField(output, std::string(prefix) + ".kind", resource.kind);
    appendField(output, std::string(prefix) + ".package", resource.package);
    appendField(output, std::string(prefix) + ".relativePath", resource.relativePath);
    appendField(output, std::string(prefix) + ".contentHash", resource.contentHash);
    appendField(output, std::string(prefix) + ".dataPlaneKey", resource.dataPlaneKey);
    appendField(output, std::string(prefix) + ".required", resource.required);
}

ViewFrameResource
getResource(const std::unordered_map<std::string, std::string>& fields, std::string_view prefix)
{
    ViewFrameResource resource;
    resource.id = getField(fields, std::string(prefix) + ".id");
    resource.kind = getField(fields, std::string(prefix) + ".kind");
    resource.package = getField(fields, std::string(prefix) + ".package");
    resource.relativePath = getField(fields, std::string(prefix) + ".relativePath");
    resource.contentHash = getField(fields, std::string(prefix) + ".contentHash");
    resource.dataPlaneKey = getField(fields, std::string(prefix) + ".dataPlaneKey");
    resource.required = getBool(fields, std::string(prefix) + ".required");
    return resource;
}

} // end unnamed namespace

std::string
serializeViewFrame(const ViewFrame& frame)
{
    std::ostringstream output;
    output << "frameId=" << frame.frameId << ';';
    output << "time=" << formatDouble(frame.time) << ';';
    output << "summary=" << escape(frame.summary) << ';';
    output << "selectionCount=" << frame.selections.size();
    appendField(output, "timeScale", frame.timeScale);
    appendField(output, "paused", frame.paused);

    appendArray(output, "camera.position", frame.camera.positionKm);
    appendArray(output, "camera.orientation", frame.camera.orientation);
    appendField(output, "camera.fovDeg", frame.camera.fovDeg);
    appendField(output, "camera.nearPlaneKm", frame.camera.nearPlaneKm);
    appendField(output, "camera.farPlaneKm", frame.camera.farPlaneKm);

    appendField(output, "observer.referenceBodyId", frame.observer.referenceBodyId);
    appendField(output, "observer.frame", frame.observer.frame);
    appendArray(output, "observer.position", frame.observer.positionKm);
    appendArray(output, "observer.velocity", frame.observer.velocityKmPerSec);

    appendField(output, "resourceCount", static_cast<std::uint64_t>(frame.resources.size()));
    for (std::size_t i = 0; i < frame.resources.size(); ++i)
        appendResource(output, "resource" + std::to_string(i), frame.resources[i]);

    appendField(output, "bodyCount", static_cast<std::uint64_t>(frame.bodies.size()));
    for (std::size_t i = 0; i < frame.bodies.size(); ++i)
    {
        const auto prefix = "body" + std::to_string(i);
        const auto& body = frame.bodies[i];
        appendField(output, prefix + ".objectId", body.objectId);
        appendField(output, prefix + ".bodyId", body.bodyId);
        appendField(output, prefix + ".name", body.name);
        appendArray(output, prefix + ".position", body.positionKm);
        appendField(output, prefix + ".radiusKm", body.radiusKm);
        appendField(output, prefix + ".visible", body.visible);
        appendField(output, prefix + ".meshResourceId", body.meshResourceId);
        appendField(output, prefix + ".diffuseTextureResourceId", body.diffuseTextureResourceId);
        appendField(output, prefix + ".normalTextureResourceId", body.normalTextureResourceId);
        appendField(output, prefix + ".material", body.material);
    }

    appendField(output, "starCount", static_cast<std::uint64_t>(frame.stars.size()));
    for (std::size_t i = 0; i < frame.stars.size(); ++i)
    {
        const auto prefix = "star" + std::to_string(i);
        const auto& star = frame.stars[i];
        appendField(output, prefix + ".objectId", star.objectId);
        appendField(output, prefix + ".starId", star.starId);
        appendField(output, prefix + ".name", star.name);
        appendArray(output, prefix + ".position", star.positionKm);
        appendField(output, prefix + ".magnitude", star.magnitude);
        appendArray(output, prefix + ".color", star.color);
        appendField(output, prefix + ".catalogResourceId", star.catalogResourceId);
    }

    appendField(output, "orbitCount", static_cast<std::uint64_t>(frame.orbits.size()));
    for (std::size_t i = 0; i < frame.orbits.size(); ++i)
    {
        const auto prefix = "orbit" + std::to_string(i);
        const auto& orbit = frame.orbits[i];
        appendField(output, prefix + ".objectId", orbit.objectId);
        appendField(output, prefix + ".bodyId", orbit.bodyId);
        appendField(output, prefix + ".visible", orbit.visible);
        appendArray(output, prefix + ".color", orbit.color);
        appendField(output, prefix + ".pointCount", static_cast<std::uint64_t>(orbit.pointsKm.size()));
        for (std::size_t point = 0; point < orbit.pointsKm.size(); ++point)
            appendArray(output, prefix + ".point" + std::to_string(point), orbit.pointsKm[point]);
    }

    for (std::size_t i = 0; i < frame.selections.size(); ++i)
    {
        const auto& selection = frame.selections[i];
        output << ";selection" << i << ".type=" << escape(selection.type);
        output << ";selection" << i << ".id=" << escape(selection.id);
        output << ";selection" << i << ".x=" << formatDouble(selection.positionKm[0]);
        output << ";selection" << i << ".y=" << formatDouble(selection.positionKm[1]);
        output << ";selection" << i << ".z=" << formatDouble(selection.positionKm[2]);
        output << ";selection" << i << ".visible=" << (selection.visible ? "true" : "false");
        output << ";selection" << i << ".clickable=" << (selection.clickable ? "true" : "false");
    }

    return output.str();
}

std::optional<ViewFrame>
deserializeViewFrame(std::string_view payload)
{
    const auto fields = parsePayload(payload);
    const auto frameId = fields.find("frameId");
    const auto time = fields.find("time");
    if (frameId == fields.end() || time == fields.end())
        return std::nullopt;

    const auto parsedFrameId = parseUint64(frameId->second);
    const auto parsedTime = parseDouble(time->second);
    if (!parsedFrameId.has_value() || !parsedTime.has_value())
        return std::nullopt;

    ViewFrame frame;
    frame.frameId = *parsedFrameId;
    frame.time = *parsedTime;
    frame.timeScale = getDouble(fields, "timeScale").value_or(1.0);
    frame.paused = getBool(fields, "paused");
    frame.camera.positionKm = getArray(fields, "camera.position", frame.camera.positionKm);
    frame.camera.orientation = getArray(fields, "camera.orientation", frame.camera.orientation);
    frame.camera.fovDeg = getDouble(fields, "camera.fovDeg").value_or(0.0);
    frame.camera.nearPlaneKm = getDouble(fields, "camera.nearPlaneKm").value_or(0.0);
    frame.camera.farPlaneKm = getDouble(fields, "camera.farPlaneKm").value_or(0.0);
    frame.observer.referenceBodyId = getField(fields, "observer.referenceBodyId");
    frame.observer.frame = getField(fields, "observer.frame");
    frame.observer.positionKm = getArray(fields, "observer.position", frame.observer.positionKm);
    frame.observer.velocityKmPerSec = getArray(fields, "observer.velocity", frame.observer.velocityKmPerSec);
    if (const auto summary = fields.find("summary"); summary != fields.end())
        frame.summary = summary->second;

    const auto resourceCount = getUint64(fields, "resourceCount");
    for (std::uint64_t i = 0; i < resourceCount; ++i)
        frame.resources.push_back(getResource(fields, "resource" + std::to_string(i)));

    const auto bodyCount = getUint64(fields, "bodyCount");
    for (std::uint64_t i = 0; i < bodyCount; ++i)
    {
        const auto prefix = "body" + std::to_string(i);
        ViewFrameBody body;
        body.objectId = getField(fields, prefix + ".objectId");
        body.bodyId = getField(fields, prefix + ".bodyId");
        body.name = getField(fields, prefix + ".name");
        body.positionKm = getArray(fields, prefix + ".position", body.positionKm);
        body.radiusKm = getDouble(fields, prefix + ".radiusKm").value_or(0.0);
        body.visible = getBool(fields, prefix + ".visible");
        body.meshResourceId = getField(fields, prefix + ".meshResourceId");
        body.diffuseTextureResourceId = getField(fields, prefix + ".diffuseTextureResourceId");
        body.normalTextureResourceId = getField(fields, prefix + ".normalTextureResourceId");
        body.material = getField(fields, prefix + ".material");
        frame.bodies.push_back(std::move(body));
    }

    const auto starCount = getUint64(fields, "starCount");
    for (std::uint64_t i = 0; i < starCount; ++i)
    {
        const auto prefix = "star" + std::to_string(i);
        ViewFrameStar star;
        star.objectId = getField(fields, prefix + ".objectId");
        star.starId = getField(fields, prefix + ".starId");
        star.name = getField(fields, prefix + ".name");
        star.positionKm = getArray(fields, prefix + ".position", star.positionKm);
        star.magnitude = getDouble(fields, prefix + ".magnitude").value_or(0.0);
        star.color = getArray(fields, prefix + ".color", star.color);
        star.catalogResourceId = getField(fields, prefix + ".catalogResourceId");
        frame.stars.push_back(std::move(star));
    }

    const auto orbitCount = getUint64(fields, "orbitCount");
    for (std::uint64_t i = 0; i < orbitCount; ++i)
    {
        const auto prefix = "orbit" + std::to_string(i);
        ViewFrameOrbit orbit;
        orbit.objectId = getField(fields, prefix + ".objectId");
        orbit.bodyId = getField(fields, prefix + ".bodyId");
        orbit.visible = getBool(fields, prefix + ".visible");
        orbit.color = getArray(fields, prefix + ".color", orbit.color);

        const auto pointCount = getUint64(fields, prefix + ".pointCount");
        for (std::uint64_t point = 0; point < pointCount; ++point)
            orbit.pointsKm.push_back(getArray(fields, prefix + ".point" + std::to_string(point), std::array<double, 3>{ 0.0, 0.0, 0.0 }));

        frame.orbits.push_back(std::move(orbit));
    }

    if (const auto selectionCount = fields.find("selectionCount"); selectionCount != fields.end())
    {
        if (const auto parsedCount = parseUint64(selectionCount->second); parsedCount.has_value())
        {
            for (std::uint64_t i = 0; i < *parsedCount; ++i)
            {
                const auto prefix = "selection" + std::to_string(i) + ".";
                ViewFrameSelection selection;
                if (const auto value = fields.find(prefix + "type"); value != fields.end())
                    selection.type = value->second;
                if (const auto value = fields.find(prefix + "id"); value != fields.end())
                    selection.id = value->second;
                if (const auto value = fields.find(prefix + "x"); value != fields.end())
                    selection.positionKm[0] = parseDouble(value->second).value_or(0.0);
                if (const auto value = fields.find(prefix + "y"); value != fields.end())
                    selection.positionKm[1] = parseDouble(value->second).value_or(0.0);
                if (const auto value = fields.find(prefix + "z"); value != fields.end())
                    selection.positionKm[2] = parseDouble(value->second).value_or(0.0);
                if (const auto value = fields.find(prefix + "visible"); value != fields.end())
                    selection.visible = parseBool(value->second);
                if (const auto value = fields.find(prefix + "clickable"); value != fields.end())
                    selection.clickable = parseBool(value->second);
                frame.selections.push_back(selection);
            }
        }
    }

    return frame;
}

} // namespace celestia::runtime
