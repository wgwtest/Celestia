// viewinput.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "viewinput.h"

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
appendField(std::ostringstream& output, std::string_view key, std::uint64_t value)
{
    appendField(output, key, std::to_string(value));
}

void
appendField(std::ostringstream& output, std::string_view key, double value)
{
    appendField(output, key, formatDouble(value));
}

std::string
getField(const std::unordered_map<std::string, std::string>& fields, std::string_view key)
{
    const auto iter = fields.find(std::string(key));
    return iter == fields.end() ? std::string{} : iter->second;
}

} // end unnamed namespace

std::string
serializeViewInputEvent(const ViewInputEvent& input)
{
    std::ostringstream output;
    appendField(output, "sessionId", input.sessionId);
    appendField(output, "sequence", input.sequence);
    appendField(output, "device", input.device);
    appendField(output, "action", input.action);
    appendField(output, "key", input.key);
    appendField(output, "pointerX", input.pointer[0]);
    appendField(output, "pointerY", input.pointer[1]);
    appendField(output, "wheelX", input.wheel[0]);
    appendField(output, "wheelY", input.wheel[1]);
    appendField(output, "modifiers", input.modifiers);
    return output.str();
}

std::optional<ViewInputEvent>
deserializeViewInputEvent(std::string_view payload)
{
    const auto fields = parsePayload(payload);
    const auto sequence = parseUint64(getField(fields, "sequence"));
    if (!sequence.has_value())
        return std::nullopt;

    ViewInputEvent input;
    input.sessionId = getField(fields, "sessionId");
    input.sequence = *sequence;
    input.device = getField(fields, "device");
    input.action = getField(fields, "action");
    input.key = getField(fields, "key");
    input.pointer[0] = parseDouble(getField(fields, "pointerX")).value_or(0.0);
    input.pointer[1] = parseDouble(getField(fields, "pointerY")).value_or(0.0);
    input.wheel[0] = parseDouble(getField(fields, "wheelX")).value_or(0.0);
    input.wheel[1] = parseDouble(getField(fields, "wheelY")).value_or(0.0);
    input.modifiers = getField(fields, "modifiers");
    return input;
}

RuntimeEnvelope
viewInputEnvelope(const ViewInputEvent& input, RuntimeRole source, RuntimeRole target)
{
    RuntimeEnvelope envelope;
    envelope.sessionId = input.sessionId;
    envelope.sequenceId = input.sequence;
    envelope.sourceRole = source;
    envelope.targetRole = target;
    envelope.kind = RuntimeMessageKind::Event;
    envelope.name = ViewInputMessageName;
    envelope.payload = serializeViewInputEvent(input);
    return envelope;
}

} // namespace celestia::runtime::protocol
