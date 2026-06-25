// sceneprotocol.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sceneprotocol.h"

#include <array>
#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace celestia::runtime::protocol
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

std::string
formatDouble(double value)
{
    std::ostringstream output;
    output << std::setprecision(17) << value;
    return output.str();
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

void
appendField(std::ostringstream& output, std::string_view key, std::string_view value)
{
    if (output.tellp() > 0)
        output << ';';
    output << key << '=' << escape(value);
}

void
appendField(std::ostringstream& output, std::string_view key, double value)
{
    appendField(output, key, formatDouble(value));
}

void
appendField(std::ostringstream& output, std::string_view key, std::uint64_t value)
{
    appendField(output, key, std::to_string(value));
}

void
appendField(std::ostringstream& output, std::string_view key, bool value)
{
    appendField(output, key, value ? std::string_view{ "true" } : std::string_view{ "false" });
}

template<std::size_t N>
void
appendArray(std::ostringstream& output, std::string_view prefix, const std::array<double, N>& values)
{
    for (std::size_t i = 0; i < N; ++i)
        appendField(output, std::string(prefix) + std::to_string(i), values[i]);
}

void
appendResource(std::ostringstream& output, std::string_view prefix, const ResourceRef& resource)
{
    appendField(output, std::string(prefix) + ".kind", resource.kind);
    appendField(output, std::string(prefix) + ".package", resource.package);
    appendField(output, std::string(prefix) + ".relativePath", resource.relativePath);
    appendField(output, std::string(prefix) + ".contentHash", resource.contentHash);
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
    if (iter == fields.end())
        return std::nullopt;
    return parseDouble(iter->second);
}

std::optional<std::uint64_t>
getUint64(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    if (iter == fields.end())
        return std::nullopt;
    return parseUint64(iter->second);
}

bool
getBool(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    return iter != fields.end() && parseBool(iter->second);
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

ResourceRef
getResource(const std::unordered_map<std::string, std::string>& fields, std::string_view prefix)
{
    ResourceRef resource;
    resource.kind = getField(fields, std::string(prefix) + ".kind");
    resource.package = getField(fields, std::string(prefix) + ".package");
    resource.relativePath = getField(fields, std::string(prefix) + ".relativePath");
    resource.contentHash = getField(fields, std::string(prefix) + ".contentHash");
    return resource;
}

} // end unnamed namespace

std::string
serializeSceneFrame(const SceneFrame& frame)
{
    std::ostringstream output;
    appendField(output, "sessionId", frame.sessionId);
    appendField(output, "sequence", frame.sequence);
    appendField(output, "simulationTime", frame.simulationTime);

    appendArray(output, "camera.position", frame.camera.position);
    appendArray(output, "camera.orientation", frame.camera.orientation);
    appendField(output, "camera.fov", frame.camera.fov);
    appendField(output, "camera.nearPlane", frame.camera.nearPlane);
    appendField(output, "camera.farPlane", frame.camera.farPlane);

    appendField(output, "observer.referenceBodyId", frame.observer.referenceBodyId);
    appendField(output, "observer.frame", frame.observer.frame);
    appendArray(output, "observer.position", frame.observer.position);
    appendArray(output, "observer.velocity", frame.observer.velocity);

    appendField(output, "render.showStars", frame.renderSettings.showStars);
    appendField(output, "render.showOrbits", frame.renderSettings.showOrbits);
    appendField(output, "render.showLabels", frame.renderSettings.showLabels);
    appendField(output, "render.ambientLight", frame.renderSettings.ambientLight);
    appendField(output, "render.exposure", frame.renderSettings.exposure);

    appendField(output, "resourceCount", static_cast<std::uint64_t>(frame.resources.size()));
    for (std::size_t i = 0; i < frame.resources.size(); ++i)
        appendResource(output, "resource" + std::to_string(i), frame.resources[i]);

    appendField(output, "bodyCount", static_cast<std::uint64_t>(frame.bodies.size()));
    for (std::size_t i = 0; i < frame.bodies.size(); ++i)
    {
        const auto prefix = "body" + std::to_string(i);
        const auto& body = frame.bodies[i];
        appendField(output, prefix + ".bodyId", body.bodyId);
        appendField(output, prefix + ".name", body.name);
        appendArray(output, prefix + ".transform", body.transform);
        appendField(output, prefix + ".radius", body.radius);
        appendField(output, prefix + ".visible", body.visible);
        appendResource(output, prefix + ".meshResource", body.meshResource);
        appendResource(output, prefix + ".diffuseTexture", body.diffuseTexture);
        appendResource(output, prefix + ".normalTexture", body.normalTexture);
        appendField(output, prefix + ".material", body.material);
    }

    appendField(output, "starCount", static_cast<std::uint64_t>(frame.stars.size()));
    for (std::size_t i = 0; i < frame.stars.size(); ++i)
    {
        const auto prefix = "star" + std::to_string(i);
        const auto& star = frame.stars[i];
        appendField(output, prefix + ".starId", star.starId);
        appendArray(output, prefix + ".position", star.position);
        appendField(output, prefix + ".magnitude", star.magnitude);
        appendArray(output, prefix + ".color", star.color);
        appendResource(output, prefix + ".catalogResource", star.catalogResource);
    }

    appendField(output, "orbitCount", static_cast<std::uint64_t>(frame.orbits.size()));
    for (std::size_t i = 0; i < frame.orbits.size(); ++i)
    {
        const auto prefix = "orbit" + std::to_string(i);
        const auto& orbit = frame.orbits[i];
        appendField(output, prefix + ".bodyId", orbit.bodyId);
        appendField(output, prefix + ".visible", orbit.visible);
        appendArray(output, prefix + ".color", orbit.color);
        appendField(output, prefix + ".pointCount", static_cast<std::uint64_t>(orbit.points.size()));
        for (std::size_t point = 0; point < orbit.points.size(); ++point)
            appendArray(output, prefix + ".point" + std::to_string(point), orbit.points[point]);
    }

    appendField(output, "labelCount", static_cast<std::uint64_t>(frame.labels.size()));
    for (std::size_t i = 0; i < frame.labels.size(); ++i)
        appendField(output, "label" + std::to_string(i), frame.labels[i]);

    appendField(output, "selection.type", frame.selection.type);
    appendField(output, "selection.id", frame.selection.id);
    return output.str();
}

std::optional<SceneFrame>
deserializeSceneFrame(std::string_view payload)
{
    const auto fields = parsePayload(payload);
    const auto sequence = getUint64(fields, "sequence");
    const auto simulationTime = getDouble(fields, "simulationTime");
    if (!sequence.has_value() || !simulationTime.has_value())
        return std::nullopt;

    SceneFrame frame;
    frame.sessionId = getField(fields, "sessionId");
    frame.sequence = *sequence;
    frame.simulationTime = *simulationTime;

    frame.camera.position = getArray(fields, "camera.position", frame.camera.position);
    frame.camera.orientation = getArray(fields, "camera.orientation", frame.camera.orientation);
    frame.camera.fov = getDouble(fields, "camera.fov").value_or(0.0);
    frame.camera.nearPlane = getDouble(fields, "camera.nearPlane").value_or(0.0);
    frame.camera.farPlane = getDouble(fields, "camera.farPlane").value_or(0.0);

    frame.observer.referenceBodyId = getField(fields, "observer.referenceBodyId");
    frame.observer.frame = getField(fields, "observer.frame");
    frame.observer.position = getArray(fields, "observer.position", frame.observer.position);
    frame.observer.velocity = getArray(fields, "observer.velocity", frame.observer.velocity);

    frame.renderSettings.showStars = getBool(fields, "render.showStars");
    frame.renderSettings.showOrbits = getBool(fields, "render.showOrbits");
    frame.renderSettings.showLabels = getBool(fields, "render.showLabels");
    frame.renderSettings.ambientLight = getDouble(fields, "render.ambientLight").value_or(0.0);
    frame.renderSettings.exposure = getDouble(fields, "render.exposure").value_or(1.0);

    const auto resourceCount = getUint64(fields, "resourceCount").value_or(0);
    for (std::uint64_t i = 0; i < resourceCount; ++i)
        frame.resources.push_back(getResource(fields, "resource" + std::to_string(i)));

    const auto bodyCount = getUint64(fields, "bodyCount").value_or(0);
    for (std::uint64_t i = 0; i < bodyCount; ++i)
    {
        const auto prefix = "body" + std::to_string(i);
        BodyRenderState body;
        body.bodyId = getField(fields, prefix + ".bodyId");
        body.name = getField(fields, prefix + ".name");
        body.transform = getArray(fields, prefix + ".transform", body.transform);
        body.radius = getDouble(fields, prefix + ".radius").value_or(0.0);
        body.visible = getBool(fields, prefix + ".visible");
        body.meshResource = getResource(fields, prefix + ".meshResource");
        body.diffuseTexture = getResource(fields, prefix + ".diffuseTexture");
        body.normalTexture = getResource(fields, prefix + ".normalTexture");
        body.material = getField(fields, prefix + ".material");
        frame.bodies.push_back(std::move(body));
    }

    const auto starCount = getUint64(fields, "starCount").value_or(0);
    for (std::uint64_t i = 0; i < starCount; ++i)
    {
        const auto prefix = "star" + std::to_string(i);
        StarRenderState star;
        star.starId = getField(fields, prefix + ".starId");
        star.position = getArray(fields, prefix + ".position", star.position);
        star.magnitude = getDouble(fields, prefix + ".magnitude").value_or(0.0);
        star.color = getArray(fields, prefix + ".color", star.color);
        star.catalogResource = getResource(fields, prefix + ".catalogResource");
        frame.stars.push_back(std::move(star));
    }

    const auto orbitCount = getUint64(fields, "orbitCount").value_or(0);
    for (std::uint64_t i = 0; i < orbitCount; ++i)
    {
        const auto prefix = "orbit" + std::to_string(i);
        OrbitRenderState orbit;
        orbit.bodyId = getField(fields, prefix + ".bodyId");
        orbit.visible = getBool(fields, prefix + ".visible");
        orbit.color = getArray(fields, prefix + ".color", orbit.color);

        const auto pointCount = getUint64(fields, prefix + ".pointCount").value_or(0);
        for (std::uint64_t point = 0; point < pointCount; ++point)
            orbit.points.push_back(getArray(fields, prefix + ".point" + std::to_string(point), std::array<double, 3>{ 0.0, 0.0, 0.0 }));

        frame.orbits.push_back(std::move(orbit));
    }

    const auto labelCount = getUint64(fields, "labelCount").value_or(0);
    for (std::uint64_t i = 0; i < labelCount; ++i)
        frame.labels.push_back(getField(fields, "label" + std::to_string(i)));

    frame.selection.type = getField(fields, "selection.type");
    frame.selection.id = getField(fields, "selection.id");
    return frame;
}

RuntimeEnvelope
sceneFrameEnvelope(const SceneFrame& frame, RuntimeRole source, RuntimeRole target)
{
    RuntimeEnvelope envelope;
    envelope.sessionId = frame.sessionId;
    envelope.sequenceId = frame.sequence;
    envelope.sourceRole = source;
    envelope.targetRole = target;
    envelope.kind = RuntimeMessageKind::ViewFrame;
    envelope.name = SceneFrameMessageName;
    envelope.payload = serializeSceneFrame(frame);
    return envelope;
}

} // namespace celestia::runtime::protocol
