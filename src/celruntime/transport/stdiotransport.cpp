// stdiotransport.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "stdiotransport.h"

#include <istream>
#include <ostream>
#include <string>

namespace celestia::runtime::transport
{

StdioTransport::StdioTransport(std::istream& input, std::ostream& output) :
    m_input(input),
    m_output(output)
{
}

bool
StdioTransport::send(const protocol::RuntimeEnvelope& envelope)
{
    if (m_closed)
        return false;

    m_output << encodeFrame(envelope);
    m_output.flush();
    return m_output.good();
}

ReceiveResult
StdioTransport::receive()
{
    if (m_closed)
        return { ReceiveStatus::Closed, std::nullopt };

    for (;;)
    {
        auto received = m_reader.receive();
        if (received.status != ReceiveStatus::WouldBlock)
            return received;

        char next = '\0';
        if (!m_input.get(next))
        {
            m_reader.close();
            return m_reader.receive();
        }

        m_reader.append(std::string_view(&next, 1));
    }
}

void
StdioTransport::close()
{
    m_closed = true;
    m_reader.close();
}

} // namespace celestia::runtime::transport
