// stdiotransport.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "framedtransport.h"

#include <iosfwd>

namespace celestia::runtime::transport
{

class StdioTransport : public FramedTransport
{
public:
    StdioTransport(std::istream&, std::ostream&);

    bool send(const protocol::RuntimeEnvelope&) override;
    ReceiveResult receive() override;
    void close() override;

private:
    std::istream& m_input;
    std::ostream& m_output;
    FramedMessageReader m_reader;
    bool m_closed{ false };
};

} // namespace celestia::runtime::transport
