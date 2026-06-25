// message.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "message.h"

#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace celestia::runtime::ipc
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

std::optional<bool>
parseBool(std::string_view text)
{
    if (text == "1")
        return true;
    if (text == "0")
        return false;
    return std::nullopt;
}

std::unordered_map<std::string, std::string>
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
        if (!line.empty())
        {
            const auto delimiter = line.find('=');
            if (delimiter != std::string_view::npos)
            {
                fields.emplace(std::string(line.substr(0, delimiter)),
                               std::string(line.substr(delimiter + 1)));
            }
        }

        offset = end + 1;
    }

    return fields;
}

const std::string*
findField(const std::unordered_map<std::string, std::string>& fields, const std::string& name)
{
    const auto iter = fields.find(name);
    return iter == fields.end() ? nullptr : &iter->second;
}

std::optional<std::string>
decodeField(const std::unordered_map<std::string, std::string>& fields, const std::string& name)
{
    const auto* value = findField(fields, name);
    return value == nullptr ? std::optional<std::string>{} : unescape(*value);
}

std::string
selectionPrefix(std::size_t index)
{
    return "selection." + std::to_string(index) + ".";
}

} // end unnamed namespace

std::string_view
messageKindName(MessageKind kind)
{
    switch (kind)
    {
    case MessageKind::Command:
        return "command";
    case MessageKind::Event:
        return "event";
    case MessageKind::ViewFrame:
        return "viewFrame";
    case MessageKind::Error:
        return "error";
    }

    return "error";
}

std::optional<MessageKind>
messageKindFromString(std::string_view text)
{
    if (text == "command")
        return MessageKind::Command;
    if (text == "event")
        return MessageKind::Event;
    if (text == "viewFrame")
        return MessageKind::ViewFrame;
    if (text == "error")
        return MessageKind::Error;
    return std::nullopt;
}

RuntimeMessage
RuntimeMessage::command(std::string name, std::string payload)
{
    RuntimeMessage message;
    message.kind = MessageKind::Command;
    message.name = std::move(name);
    message.payload = std::move(payload);
    return message;
}

RuntimeMessage
RuntimeMessage::event(std::string name, std::string payload)
{
    RuntimeMessage message;
    message.kind = MessageKind::Event;
    message.name = std::move(name);
    message.payload = std::move(payload);
    return message;
}

RuntimeMessage
RuntimeMessage::viewFrame(std::string name, celestia::runtime::ViewFrame frame)
{
    RuntimeMessage message;
    message.kind = MessageKind::ViewFrame;
    message.name = std::move(name);
    message.frame = std::move(frame);
    return message;
}

RuntimeMessage
RuntimeMessage::error(std::string name, std::string payload)
{
    RuntimeMessage message;
    message.kind = MessageKind::Error;
    message.name = std::move(name);
    message.payload = std::move(payload);
    return message;
}

std::string
serializeMessage(const RuntimeMessage& message)
{
    std::ostringstream output;
    output << "protocolVersion=" << message.protocolVersion << '\n'
           << "kind=" << messageKindName(message.kind) << '\n'
           << "name=" << escape(message.name) << '\n'
           << "payload=" << escape(message.payload) << '\n';

    if (message.kind == MessageKind::ViewFrame)
    {
        output << "frame.time=" << formatDouble(message.frame.time) << '\n'
               << "frame.selectionCount=" << message.frame.selections.size() << '\n';

        for (std::size_t i = 0; i < message.frame.selections.size(); ++i)
        {
            const auto& selection = message.frame.selections[i];
            const auto prefix = selectionPrefix(i);
            output << prefix << "type=" << escape(selection.type) << '\n'
                   << prefix << "id=" << escape(selection.id) << '\n'
                   << prefix << "positionKm.0=" << formatDouble(selection.positionKm[0]) << '\n'
                   << prefix << "positionKm.1=" << formatDouble(selection.positionKm[1]) << '\n'
                   << prefix << "positionKm.2=" << formatDouble(selection.positionKm[2]) << '\n'
                   << prefix << "visible=" << (selection.visible ? '1' : '0') << '\n'
                   << prefix << "clickable=" << (selection.clickable ? '1' : '0') << '\n';
        }
    }

    return output.str();
}

std::optional<RuntimeMessage>
deserializeMessage(std::string_view serialized)
{
    const auto fields = parseFields(serialized);

    const auto* versionField = findField(fields, "protocolVersion");
    if (versionField == nullptr)
        return std::nullopt;

    const auto protocolVersion = parseInt(*versionField);
    if (!protocolVersion.has_value() || *protocolVersion != CurrentProtocolVersion)
        return std::nullopt;

    const auto* kindField = findField(fields, "kind");
    if (kindField == nullptr)
        return std::nullopt;

    const auto kind = messageKindFromString(*kindField);
    if (!kind.has_value())
        return std::nullopt;

    auto name = decodeField(fields, "name");
    auto payload = decodeField(fields, "payload");
    if (!name.has_value() || !payload.has_value())
        return std::nullopt;

    RuntimeMessage message;
    message.protocolVersion = *protocolVersion;
    message.kind = *kind;
    message.name = std::move(*name);
    message.payload = std::move(*payload);

    if (message.kind == MessageKind::ViewFrame)
    {
        const auto* timeField = findField(fields, "frame.time");
        const auto* countField = findField(fields, "frame.selectionCount");
        if (timeField == nullptr || countField == nullptr)
            return std::nullopt;

        const auto time = parseDouble(*timeField);
        const auto selectionCount = parseSize(*countField);
        if (!time.has_value() || !selectionCount.has_value())
            return std::nullopt;

        message.frame.time = *time;
        message.frame.selections.clear();
        message.frame.selections.reserve(*selectionCount);

        for (std::size_t i = 0; i < *selectionCount; ++i)
        {
            const auto prefix = selectionPrefix(i);
            auto type = decodeField(fields, prefix + "type");
            auto id = decodeField(fields, prefix + "id");
            const auto* xField = findField(fields, prefix + "positionKm.0");
            const auto* yField = findField(fields, prefix + "positionKm.1");
            const auto* zField = findField(fields, prefix + "positionKm.2");
            const auto* visibleField = findField(fields, prefix + "visible");
            const auto* clickableField = findField(fields, prefix + "clickable");
            if (!type.has_value() || !id.has_value() ||
                xField == nullptr || yField == nullptr || zField == nullptr ||
                visibleField == nullptr || clickableField == nullptr)
            {
                return std::nullopt;
            }

            const auto x = parseDouble(*xField);
            const auto y = parseDouble(*yField);
            const auto z = parseDouble(*zField);
            const auto visible = parseBool(*visibleField);
            const auto clickable = parseBool(*clickableField);
            if (!x.has_value() || !y.has_value() || !z.has_value() ||
                !visible.has_value() || !clickable.has_value())
            {
                return std::nullopt;
            }

            celestia::runtime::ViewFrameSelection selection;
            selection.type = std::move(*type);
            selection.id = std::move(*id);
            selection.positionKm = { *x, *y, *z };
            selection.visible = *visible;
            selection.clickable = *clickable;
            message.frame.selections.push_back(std::move(selection));
        }
    }

    return message;
}

} // namespace celestia::runtime::ipc
