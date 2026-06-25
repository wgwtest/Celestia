// framedtransport.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "framedmessage.h"

namespace celestia::runtime::transport
{

class FramedTransport
{
public:
    virtual ~FramedTransport() = default;

    virtual bool send(const protocol::RuntimeEnvelope&) = 0;
    virtual ReceiveResult receive() = 0;
    virtual void close() = 0;
};

} // namespace celestia::runtime::transport
