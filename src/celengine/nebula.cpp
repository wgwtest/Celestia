// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <celutil/associativearray.h>
#include <celutil/gettext.h>
#include <celutil/stringutils.h>
#include <fmt/format.h>
#include "nebula.h"
#include "nebulalifecycle.h"

namespace util = celestia::util;

using namespace std::string_view_literals;

namespace
{
struct NebulaTypeName
{
    const char* name;
    Nebula::Type type;
};

constexpr std::array NebulaTypeNames =
{
    NebulaTypeName{ " ", Nebula::Type::NotDefined },
    NebulaTypeName{ "Emission", Nebula::Type::Emission },
    NebulaTypeName{ "Reflection",  Nebula::Type::Reflection },
    NebulaTypeName{ "Dark",  Nebula::Type::Dark },
    NebulaTypeName{ "Planetary",  Nebula::Type::Planetary },
    NebulaTypeName{ "SupernovaRemnant",  Nebula::Type::SupernovaRemnant },
    NebulaTypeName{ "HII_Region", Nebula::Type::HII_Region },
    NebulaTypeName{ "Protoplanetary", Nebula::Type::Protoplanetary },
};

}

Nebula::~Nebula()
{
    NebulaLifecycleEvents::notifyDestroyed(this);
}

const char*
Nebula::getType() const
{
    return NebulaTypeNames[static_cast<std::size_t>(type)].name;
}

void
Nebula::setType(const std::string& typeStr)
{
    type = Nebula::Type::NotDefined;
    auto iter = std::find_if(std::begin(NebulaTypeNames), std::end(NebulaTypeNames),
                             [&](const NebulaTypeName& n) { return compareIgnoringCase(n.name, typeStr) == 0; });
    if (iter != std::end(NebulaTypeNames))
        type = iter->type;
}

std::string
Nebula::getDescription() const
{
    return fmt::format(_("Nebula: {}"), getType());
}

DeepSkyObjectType
Nebula::getObjType() const
{
    return DeepSkyObjectType::Nebula;
}

bool
Nebula::loadDetails(const util::AssociativeArray* params,
                    const std::filesystem::path&)
{
    if (const std::string* typeName = params->getString("Type"); typeName != nullptr)
        setType(*typeName);

    return true;
}

Nebula::Type
Nebula::getNebulaType() const
{
    return type;
}
