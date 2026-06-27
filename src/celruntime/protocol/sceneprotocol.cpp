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
#include <cmath>
#include <iomanip>
#include <limits>
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

std::optional<int>
parseInt(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoi(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return value;
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
    appendField(output, std::string(prefix) + ".id", resource.id);
    appendField(output, std::string(prefix) + ".kind", resource.kind);
    appendField(output, std::string(prefix) + ".package", resource.package);
    appendField(output, std::string(prefix) + ".relativePath", resource.relativePath);
    appendField(output, std::string(prefix) + ".contentHash", resource.contentHash);
    appendField(output, std::string(prefix) + ".dataPlaneKey", resource.dataPlaneKey);
    appendField(output, std::string(prefix) + ".required", resource.required);
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

std::optional<int>
getInt(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    if (iter == fields.end())
        return std::nullopt;
    return parseInt(iter->second);
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
    resource.id = getField(fields, std::string(prefix) + ".id");
    resource.kind = getField(fields, std::string(prefix) + ".kind");
    resource.package = getField(fields, std::string(prefix) + ".package");
    resource.relativePath = getField(fields, std::string(prefix) + ".relativePath");
    resource.contentHash = getField(fields, std::string(prefix) + ".contentHash");
    resource.dataPlaneKey = getField(fields, std::string(prefix) + ".dataPlaneKey");
    resource.required = getBool(fields, std::string(prefix) + ".required");
    return resource;
}

bool
containsForbiddenToken(std::string_view text)
{
    constexpr std::array<std::string_view, 9> Forbidden{
        "Simulation*",
        "Universe*",
        "CelestiaCore*",
        "Renderer*",
        "Texture*",
        "Mesh*",
        "GLuint",
        "0x",
        ":\\",
    };

    for (const auto token : Forbidden)
    {
        if (text.find(token) != std::string_view::npos)
            return true;
    }

    return false;
}

bool
startsWithSlash(std::string_view text)
{
    return !text.empty() && (text.front() == '/' || text.front() == '\\');
}

bool
containsTraversal(std::string_view path)
{
    if (path == ".." || path.find("../") != std::string_view::npos ||
        path.find("..\\") != std::string_view::npos)
    {
        return true;
    }

    return false;
}

bool
isSafeRelativePath(std::string_view path)
{
    if (path.empty())
        return true;

    return !startsWithSlash(path) &&
           path.find(":\\") == std::string_view::npos &&
           !containsTraversal(path);
}

bool
hasResourceFields(const ResourceRef& resource)
{
    return !resource.id.empty() ||
           !resource.kind.empty() ||
           !resource.package.empty() ||
           !resource.relativePath.empty() ||
           !resource.contentHash.empty() ||
           !resource.dataPlaneKey.empty() ||
           resource.required;
}

std::string_view
stableObjectId(std::string_view preferred, std::string_view fallback)
{
    return preferred.empty() ? fallback : preferred;
}

void
addIssue(std::vector<SceneFrameValidationIssue>& issues, std::string field, std::string message)
{
    issues.push_back(SceneFrameValidationIssue{ std::move(field), std::move(message) });
}

void
validateText(std::vector<SceneFrameValidationIssue>& issues, std::string field, std::string_view value)
{
    if (containsForbiddenToken(value))
        addIssue(issues, std::move(field), "contains a process-local token");
}

void
validateResource(std::vector<SceneFrameValidationIssue>& issues,
                 const ResourceRef& resource,
                 std::string field)
{
    if (!hasResourceFields(resource))
        return;

    validateText(issues, field + ".id", resource.id);
    validateText(issues, field + ".kind", resource.kind);
    validateText(issues, field + ".package", resource.package);
    validateText(issues, field + ".relativePath", resource.relativePath);
    validateText(issues, field + ".contentHash", resource.contentHash);
    validateText(issues, field + ".dataPlaneKey", resource.dataPlaneKey);

    if (resource.relativePath.empty())
        addIssue(issues, field + ".relativePath", "must not be empty when a resource is present");
    else if (!isSafeRelativePath(resource.relativePath))
        addIssue(issues, field + ".relativePath", "must be a safe relative path");
}

} // end unnamed namespace

std::string
serializeSceneFrame(const SceneFrame& frame)
{
    std::ostringstream output;
    appendField(output, "protocolVersion", static_cast<std::uint64_t>(frame.protocolVersion));
    appendField(output, "sessionId", frame.sessionId);
    appendField(output, "sequence", frame.sequence);
    appendField(output, "simulationTime", frame.simulationTime);
    appendField(output, "time.julianDayTdb", frame.time.julianDayTdb);
    appendField(output, "time.secondsSinceJ2000", frame.time.secondsSinceJ2000);
    appendField(output, "time.timeScale", frame.time.timeScale);
    appendField(output, "time.paused", frame.time.paused);

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
        appendField(output, prefix + ".objectId", body.objectId);
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
        appendField(output, prefix + ".objectId", star.objectId);
        appendField(output, prefix + ".starId", star.starId);
        appendArray(output, prefix + ".position", star.position);
        appendField(output, prefix + ".magnitude", star.magnitude);
        appendArray(output, prefix + ".color", star.color);
        appendResource(output, prefix + ".catalogResource", star.catalogResource);
    }

    appendField(output, "dsoCount", static_cast<std::uint64_t>(frame.deepSkyObjects.size()));
    for (std::size_t i = 0; i < frame.deepSkyObjects.size(); ++i)
    {
        const auto prefix = "dso" + std::to_string(i);
        const auto& dso = frame.deepSkyObjects[i];
        appendField(output, prefix + ".objectId", dso.objectId);
        appendField(output, prefix + ".name", dso.name);
        appendField(output, prefix + ".dsoType", dso.dsoType);
        appendArray(output, prefix + ".position", dso.positionKm);
        appendField(output, prefix + ".apparentMagnitude", dso.apparentMagnitude);
        appendField(output, prefix + ".visible", dso.visible);
        appendResource(output, prefix + ".catalogResource", dso.catalogResource);
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
        appendField(output, prefix + ".pointCount", static_cast<std::uint64_t>(orbit.points.size()));
        for (std::size_t point = 0; point < orbit.points.size(); ++point)
            appendArray(output, prefix + ".point" + std::to_string(point), orbit.points[point]);
    }

    appendField(output, "labelCount", static_cast<std::uint64_t>(frame.labels.size()));
    for (std::size_t i = 0; i < frame.labels.size(); ++i)
    {
        const auto prefix = "label" + std::to_string(i);
        const auto& label = frame.labels[i];
        appendField(output, prefix, label.text);
        appendField(output, prefix + ".targetObjectId", label.targetObjectId);
        appendField(output, prefix + ".text", label.text);
        appendField(output, prefix + ".kind", label.kind);
        appendField(output, prefix + ".visible", label.visible);
    }

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
    frame.protocolVersion = getInt(fields, "protocolVersion").value_or(1);
    frame.sessionId = getField(fields, "sessionId");
    frame.sequence = *sequence;
    frame.simulationTime = *simulationTime;
    const auto structuredTime = getDouble(fields, "time.julianDayTdb");
    if (structuredTime.has_value())
        frame.time.julianDayTdb = *structuredTime;
    else if (frame.protocolVersion == 1)
        frame.time.julianDayTdb = frame.simulationTime;
    else
        frame.time.julianDayTdb = std::numeric_limits<double>::quiet_NaN();
    frame.time.secondsSinceJ2000 = getDouble(fields, "time.secondsSinceJ2000").value_or(0.0);
    frame.time.timeScale = getDouble(fields, "time.timeScale").value_or(1.0);
    frame.time.paused = getBool(fields, "time.paused");

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
        body.objectId = getField(fields, prefix + ".objectId");
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
        star.objectId = getField(fields, prefix + ".objectId");
        star.starId = getField(fields, prefix + ".starId");
        star.position = getArray(fields, prefix + ".position", star.position);
        star.magnitude = getDouble(fields, prefix + ".magnitude").value_or(0.0);
        star.color = getArray(fields, prefix + ".color", star.color);
        star.catalogResource = getResource(fields, prefix + ".catalogResource");
        frame.stars.push_back(std::move(star));
    }

    const auto dsoCount = getUint64(fields, "dsoCount").value_or(0);
    for (std::uint64_t i = 0; i < dsoCount; ++i)
    {
        const auto prefix = "dso" + std::to_string(i);
        DeepSkyRenderState dso;
        dso.objectId = getField(fields, prefix + ".objectId");
        dso.name = getField(fields, prefix + ".name");
        dso.dsoType = getField(fields, prefix + ".dsoType");
        dso.positionKm = getArray(fields, prefix + ".position", dso.positionKm);
        dso.apparentMagnitude = getDouble(fields, prefix + ".apparentMagnitude").value_or(0.0);
        dso.visible = getBool(fields, prefix + ".visible");
        dso.catalogResource = getResource(fields, prefix + ".catalogResource");
        frame.deepSkyObjects.push_back(std::move(dso));
    }

    const auto orbitCount = getUint64(fields, "orbitCount").value_or(0);
    for (std::uint64_t i = 0; i < orbitCount; ++i)
    {
        const auto prefix = "orbit" + std::to_string(i);
        OrbitRenderState orbit;
        orbit.objectId = getField(fields, prefix + ".objectId");
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
    {
        const auto prefix = "label" + std::to_string(i);
        LabelRenderState label;
        label.targetObjectId = getField(fields, prefix + ".targetObjectId");
        label.text = getField(fields, prefix + ".text");
        if (label.text.empty())
            label.text = getField(fields, prefix);
        label.kind = getField(fields, prefix + ".kind");
        label.visible = fields.find(prefix + ".visible") == fields.end() ? true : getBool(fields, prefix + ".visible");
        frame.labels.push_back(std::move(label));
    }

    frame.selection.type = getField(fields, "selection.type");
    frame.selection.id = getField(fields, "selection.id");
    return frame;
}

std::vector<SceneFrameValidationIssue>
validateSceneFrame(const SceneFrame& frame)
{
    std::vector<SceneFrameValidationIssue> issues;

    if (frame.protocolVersion != 1 && frame.protocolVersion != SceneFrameProtocolVersion)
        addIssue(issues, "protocolVersion", "must be 1 or SceneFrameProtocolVersion");

    if (frame.sessionId.empty())
        addIssue(issues, "sessionId", "must not be empty");
    validateText(issues, "sessionId", frame.sessionId);

    if (frame.protocolVersion == SceneFrameProtocolVersion &&
        !std::isfinite(frame.time.julianDayTdb))
    {
        addIssue(issues, "time.julianDayTdb", "must be finite");
    }

    if (frame.camera.fov <= 0.0)
        addIssue(issues, "camera.fov", "must be positive");
    if (frame.camera.nearPlane <= 0.0)
        addIssue(issues, "camera.nearPlane", "must be positive");
    if (frame.camera.farPlane <= frame.camera.nearPlane)
        addIssue(issues, "camera.farPlane", "must be greater than nearPlane");

    if (frame.observer.frame.empty())
        addIssue(issues, "observer.frame", "must not be empty");
    validateText(issues, "observer.referenceBodyId", frame.observer.referenceBodyId);
    validateText(issues, "observer.frame", frame.observer.frame);

    for (std::size_t i = 0; i < frame.resources.size(); ++i)
        validateResource(issues, frame.resources[i], "resource" + std::to_string(i));

    for (std::size_t i = 0; i < frame.bodies.size(); ++i)
    {
        const auto field = "body" + std::to_string(i);
        const auto id = stableObjectId(frame.bodies[i].objectId, frame.bodies[i].bodyId);
        if (id.empty())
            addIssue(issues, field + ".objectId", "must not be empty");
        validateText(issues, field + ".objectId", id);
        validateText(issues, field + ".name", frame.bodies[i].name);
        validateResource(issues, frame.bodies[i].meshResource, field + ".meshResource");
        validateResource(issues, frame.bodies[i].diffuseTexture, field + ".diffuseTexture");
        validateResource(issues, frame.bodies[i].normalTexture, field + ".normalTexture");
    }

    for (std::size_t i = 0; i < frame.stars.size(); ++i)
    {
        const auto field = "star" + std::to_string(i);
        const auto id = stableObjectId(frame.stars[i].objectId, frame.stars[i].starId);
        if (id.empty())
            addIssue(issues, field + ".objectId", "must not be empty");
        validateText(issues, field + ".objectId", id);
        validateResource(issues, frame.stars[i].catalogResource, field + ".catalogResource");
    }

    for (std::size_t i = 0; i < frame.deepSkyObjects.size(); ++i)
    {
        const auto field = "dso" + std::to_string(i);
        if (frame.deepSkyObjects[i].objectId.empty())
            addIssue(issues, field + ".objectId", "must not be empty");
        validateText(issues, field + ".objectId", frame.deepSkyObjects[i].objectId);
        validateText(issues, field + ".name", frame.deepSkyObjects[i].name);
        validateText(issues, field + ".dsoType", frame.deepSkyObjects[i].dsoType);
        validateResource(issues, frame.deepSkyObjects[i].catalogResource, field + ".catalogResource");
    }

    for (std::size_t i = 0; i < frame.orbits.size(); ++i)
    {
        const auto field = "orbit" + std::to_string(i);
        const auto id = stableObjectId(frame.orbits[i].objectId, frame.orbits[i].bodyId);
        if (id.empty())
            addIssue(issues, field + ".objectId", "must not be empty");
        validateText(issues, field + ".objectId", id);
    }

    for (std::size_t i = 0; i < frame.labels.size(); ++i)
    {
        const auto field = "label" + std::to_string(i);
        validateText(issues, field + ".targetObjectId", frame.labels[i].targetObjectId);
        validateText(issues, field + ".text", frame.labels[i].text);
        validateText(issues, field + ".kind", frame.labels[i].kind);
    }

    validateText(issues, "selection.type", frame.selection.type);
    validateText(issues, "selection.id", frame.selection.id);
    return issues;
}

bool
isValidSceneFrame(const SceneFrame& frame)
{
    return validateSceneFrame(frame).empty();
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
