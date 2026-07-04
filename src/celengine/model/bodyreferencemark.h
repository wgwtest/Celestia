// bodyreferencemark.h
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>

class BodyReferenceMark
{
public:
    BodyReferenceMark() = default;
    virtual ~BodyReferenceMark() = default;

    virtual float boundingSphereRadius() const = 0;

    void setTag(std::string_view tag)
    {
        if (tag.empty() || tag == defaultTag())
            m_tag = std::string{};
        else
            m_tag = tag;
    }

    std::string_view getTag() const
    {
        if (m_tag.empty())
            return defaultTag();
        return m_tag;
    }

protected:
    virtual std::string_view defaultTag() const = 0;

private:
    std::string m_tag;
};
