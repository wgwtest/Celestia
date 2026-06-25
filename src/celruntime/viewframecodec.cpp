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

} // end unnamed namespace

std::string
serializeViewFrame(const ViewFrame& frame)
{
    std::ostringstream output;
    output << "frameId=" << frame.frameId << ';';
    output << "time=" << formatDouble(frame.time) << ';';
    output << "summary=" << escape(frame.summary) << ';';
    output << "selectionCount=" << frame.selections.size();

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
    if (const auto summary = fields.find("summary"); summary != fields.end())
        frame.summary = summary->second;

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
