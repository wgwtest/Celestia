// framedmessage.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "framedmessage.h"

#include <sstream>
#include <string>

namespace celestia::runtime::transport
{
namespace
{

constexpr std::string_view ContentLengthPrefix{ "Content-Length: " };

std::optional<std::size_t>
parseContentLength(std::string_view header)
{
    if (header.substr(0, ContentLengthPrefix.size()) != ContentLengthPrefix)
        return std::nullopt;

    auto lengthText = header.substr(ContentLengthPrefix.size());
    if (!lengthText.empty() && lengthText.back() == '\r')
        lengthText.remove_suffix(1);

    try
    {
        std::size_t consumed = 0;
        const auto length = std::stoull(std::string(lengthText), &consumed);
        if (consumed != lengthText.size())
            return std::nullopt;
        return static_cast<std::size_t>(length);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

ReceiveResult
result(ReceiveStatus status)
{
    ReceiveResult value;
    value.status = status;
    return value;
}

ReceiveResult
messageResult(protocol::RuntimeEnvelope message)
{
    ReceiveResult value;
    value.status = ReceiveStatus::Message;
    value.message = std::move(message);
    return value;
}

} // end unnamed namespace

std::string
encodeFrame(const protocol::RuntimeEnvelope& envelope)
{
    const auto body = protocol::serializeEnvelope(envelope);
    std::ostringstream output;
    output << "Content-Length: " << body.size() << "\n\n";
    output << body;
    return output.str();
}

void
FramedMessageReader::append(std::string_view text)
{
    if (!m_closed)
        m_buffer.append(text);
}

ReceiveResult
FramedMessageReader::receive()
{
    if (m_malformed)
        return result(ReceiveStatus::Malformed);

    const auto separator = m_buffer.find("\n\n");
    if (separator == std::string::npos)
        return m_closed ? result(ReceiveStatus::Closed) : result(ReceiveStatus::WouldBlock);

    const auto contentLength = parseContentLength(std::string_view(m_buffer).substr(0, separator));
    if (!contentLength.has_value())
    {
        m_malformed = true;
        return result(ReceiveStatus::Malformed);
    }

    const auto bodyOffset = separator + 2;
    if (m_buffer.size() < bodyOffset + *contentLength)
        return m_closed ? result(ReceiveStatus::Closed) : result(ReceiveStatus::WouldBlock);

    const auto body = m_buffer.substr(bodyOffset, *contentLength);
    m_buffer.erase(0, bodyOffset + *contentLength);

    auto envelope = protocol::deserializeEnvelope(body);
    if (!envelope.has_value())
    {
        m_malformed = true;
        return result(ReceiveStatus::Malformed);
    }

    return messageResult(std::move(*envelope));
}

void
FramedMessageReader::close()
{
    m_closed = true;
}

} // namespace celestia::runtime::transport
