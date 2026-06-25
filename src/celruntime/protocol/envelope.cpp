// envelope.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "envelope.h"

#include "lifecycle.h"

#include <array>
#include <cctype>
#include <sstream>
#include <string>
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

std::optional<std::int64_t>
parseInt64(std::string_view text)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoll(std::string(text), &consumed);
        if (consumed != text.size())
            return std::nullopt;
        return static_cast<std::int64_t>(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::unordered_map<std::string, std::string>>
parseFields(std::string_view serialized)
{
    std::unordered_map<std::string, std::string> fields;
    std::size_t offset = 0;

    while (offset < serialized.size())
    {
        auto end = serialized.find('\n', offset);
        if (end == std::string_view::npos)
            end = serialized.size();

        const auto line = serialized.substr(offset, end - offset);
        offset = end + 1;
        if (line.empty())
            continue;

        const auto separator = line.find('=');
        if (separator == std::string_view::npos)
            return std::nullopt;

        const auto key = std::string(line.substr(0, separator));
        const auto value = unescape(line.substr(separator + 1));
        if (!value.has_value())
            return std::nullopt;

        fields[key] = *value;
    }

    return fields;
}

std::optional<std::string>
field(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    if (iter == fields.end())
        return std::nullopt;
    return iter->second;
}

void
appendField(std::ostringstream& output, std::string_view key, std::string_view value)
{
    output << key << '=' << escape(value) << '\n';
}

} // end unnamed namespace

std::string_view
roleName(RuntimeRole role)
{
    switch (role)
    {
    case RuntimeRole::Launcher:
        return "launcher";
    case RuntimeRole::Model:
        return "model";
    case RuntimeRole::Controller:
        return "controller";
    case RuntimeRole::View:
        return "view";
    case RuntimeRole::Broadcast:
        return "broadcast";
    }

    return "unknown";
}

std::optional<RuntimeRole>
roleFromString(std::string_view text)
{
    if (text == "launcher")
        return RuntimeRole::Launcher;
    if (text == "model")
        return RuntimeRole::Model;
    if (text == "controller")
        return RuntimeRole::Controller;
    if (text == "view")
        return RuntimeRole::View;
    if (text == "broadcast")
        return RuntimeRole::Broadcast;
    return std::nullopt;
}

std::string_view
messageKindName(RuntimeMessageKind kind)
{
    switch (kind)
    {
    case RuntimeMessageKind::Lifecycle:
        return "lifecycle";
    case RuntimeMessageKind::Command:
        return "command";
    case RuntimeMessageKind::Event:
        return "event";
    case RuntimeMessageKind::ViewFrame:
        return "viewFrame";
    case RuntimeMessageKind::Heartbeat:
        return "heartbeat";
    case RuntimeMessageKind::Error:
        return "error";
    }

    return "unknown";
}

std::optional<RuntimeMessageKind>
messageKindFromString(std::string_view text)
{
    if (text == "lifecycle")
        return RuntimeMessageKind::Lifecycle;
    if (text == "command")
        return RuntimeMessageKind::Command;
    if (text == "event")
        return RuntimeMessageKind::Event;
    if (text == "viewFrame")
        return RuntimeMessageKind::ViewFrame;
    if (text == "heartbeat")
        return RuntimeMessageKind::Heartbeat;
    if (text == "error")
        return RuntimeMessageKind::Error;
    return std::nullopt;
}

RuntimeEnvelope
lifecycle(RuntimeRole source, RuntimeRole target, std::string name)
{
    RuntimeEnvelope envelope;
    envelope.sourceRole = source;
    envelope.targetRole = target;
    envelope.kind = RuntimeMessageKind::Lifecycle;
    envelope.name = std::move(name);
    return envelope;
}

RuntimeEnvelope
hello(RuntimeRole source, RuntimeRole target)
{
    return lifecycle(source, target, RuntimeHello);
}

RuntimeEnvelope
ready(RuntimeRole source, RuntimeRole target)
{
    return lifecycle(source, target, RuntimeReady);
}

RuntimeEnvelope
shutdown(RuntimeRole source, RuntimeRole target)
{
    return lifecycle(source, target, RuntimeShutdown);
}

std::string
serializeEnvelope(const RuntimeEnvelope& envelope)
{
    std::ostringstream output;
    output << "protocolVersion=" << envelope.protocolVersion << '\n';
    appendField(output, "sessionId", envelope.sessionId);
    output << "sequenceId=" << envelope.sequenceId << '\n';
    output << "timestampUs=" << envelope.timestampUs << '\n';
    appendField(output, "sourceRole", roleName(envelope.sourceRole));
    appendField(output, "targetRole", roleName(envelope.targetRole));
    appendField(output, "kind", messageKindName(envelope.kind));
    appendField(output, "name", envelope.name);
    appendField(output, "payload", envelope.payload);
    return output.str();
}

std::optional<RuntimeEnvelope>
deserializeEnvelope(std::string_view serialized)
{
    const auto fields = parseFields(serialized);
    if (!fields.has_value())
        return std::nullopt;

    const auto protocolText = field(*fields, "protocolVersion");
    if (!protocolText.has_value())
        return std::nullopt;

    const auto protocolVersion = parseInt(*protocolText);
    if (!protocolVersion.has_value() || *protocolVersion != CurrentProtocolVersion)
        return std::nullopt;

    const auto sourceRoleText = field(*fields, "sourceRole");
    const auto targetRoleText = field(*fields, "targetRole");
    const auto kindText = field(*fields, "kind");
    const auto nameText = field(*fields, "name");
    if (!sourceRoleText.has_value() || !targetRoleText.has_value() ||
        !kindText.has_value() || !nameText.has_value())
    {
        return std::nullopt;
    }

    const auto sourceRole = roleFromString(*sourceRoleText);
    const auto targetRole = roleFromString(*targetRoleText);
    const auto kind = messageKindFromString(*kindText);
    if (!sourceRole.has_value() || !targetRole.has_value() || !kind.has_value())
        return std::nullopt;

    RuntimeEnvelope envelope;
    envelope.protocolVersion = *protocolVersion;
    envelope.sourceRole = *sourceRole;
    envelope.targetRole = *targetRole;
    envelope.kind = *kind;
    envelope.name = *nameText;

    if (const auto sessionId = field(*fields, "sessionId"); sessionId.has_value())
        envelope.sessionId = *sessionId;
    if (const auto sequenceId = field(*fields, "sequenceId"); sequenceId.has_value())
    {
        const auto parsed = parseUint64(*sequenceId);
        if (!parsed.has_value())
            return std::nullopt;
        envelope.sequenceId = *parsed;
    }
    if (const auto timestampUs = field(*fields, "timestampUs"); timestampUs.has_value())
    {
        const auto parsed = parseInt64(*timestampUs);
        if (!parsed.has_value())
            return std::nullopt;
        envelope.timestampUs = *parsed;
    }
    if (const auto payload = field(*fields, "payload"); payload.has_value())
        envelope.payload = *payload;

    return envelope;
}

} // namespace celestia::runtime::protocol
