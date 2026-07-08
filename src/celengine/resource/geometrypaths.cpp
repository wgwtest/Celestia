// geometrypaths.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/resource/geometrypaths.h>

#include <cstddef>
#include <system_error>
#include <tuple>

#include <boost/container_hash/hash.hpp>

#include <celutil/logger.h>

using celestia::util::GetLogger;

namespace celestia::engine
{

GeometryPaths::Info::Info(PathIndex _pathIndex,
                          PathIndex _directoryPathIndex,
                          const Eigen::Vector3f& _center,
                          bool _isNormalized) :
    pathIndex(_pathIndex),
    directoryPathIndex(_directoryPathIndex),
    center(_center),
    isNormalized(_isNormalized)
{
}

GeometryPaths::Key::Key(PathIndex _pathIndex,
                        const Eigen::Vector3f& _center,
                        bool _isNormalized) :
    pathIndex(_pathIndex),
    center(_center),
    isNormalized(_isNormalized)
{
}

std::size_t
GeometryPaths::KeyHash::operator()(const Key& key) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, key.pathIndex);
    boost::hash_combine(seed, key.center.x());
    boost::hash_combine(seed, key.center.y());
    boost::hash_combine(seed, key.center.z());
    boost::hash_combine(seed, key.isNormalized);
    return seed;
}

bool
GeometryPaths::KeyEqual::operator()(const Key& lhs, const Key& rhs) const
{
    return std::tie(lhs.pathIndex, lhs.center, lhs.isNormalized) ==
           std::tie(rhs.pathIndex, rhs.center, rhs.isNormalized);
}

GeometryHandle
GeometryPaths::getHandle(const std::filesystem::path& filename,
                         const std::filesystem::path& directory,
                         const Eigen::Vector3f& center,
                         bool isNormalized)
{
    if (filename.empty())
        return GeometryHandle::Empty;

    PathIndex directoryPathIndex = getPathIndex(directory);
    PathIndex fileIndex = getFileIndex(directoryPathIndex, filename);
    if (fileIndex == PathIndex::Invalid)
        return GeometryHandle::Empty;

    auto [it, inserted] = m_handles.try_emplace(Key(fileIndex, center, isNormalized),
                                                static_cast<GeometryHandle>(m_info.size()));
    if (inserted)
        m_info.emplace_back(fileIndex, directoryPathIndex, center, isNormalized);

    return it->second;
}

GeometryPaths::PathIndex
GeometryPaths::getFileIndex(PathIndex& directoryPathIndex,
                            const std::filesystem::path& filename)
{
    DirectoryPaths& dirPaths = m_dirPaths.try_emplace(directoryPathIndex).first->second;
    auto [it, inserted] = dirPaths.try_emplace(filename, PathIndex::Invalid);
    if (!inserted ||
        checkPath(directoryPathIndex, filename, it->second) ||
        directoryPathIndex == PathIndex::Root)
    {
        return it->second;
    }

    DirectoryPaths& rootPaths = m_dirPaths.try_emplace(PathIndex::Root).first->second;
    auto [rootIt, rootInserted] = rootPaths.try_emplace(filename, PathIndex::Invalid);
    if (!rootInserted || checkPath(PathIndex::Root, filename, rootIt->second))
    {
        directoryPathIndex = PathIndex::Root;
        it->second = rootIt->second;
    }

    GetLogger()->error("Failed to resolve model file {}\n", filename);
    return it->second;
}

bool
GeometryPaths::checkPath(PathIndex directoryPathIndex,
                         const std::filesystem::path& filename,
                         PathIndex& pathIndex)
{
    std::filesystem::path filePath = directoryPathIndex == PathIndex::Root
        ? "models" / filename
        : m_paths[static_cast<std::size_t>(directoryPathIndex)] / "models" / filename;

    std::error_code ec;
    if (auto status = std::filesystem::status(filePath, ec);
        ec || !std::filesystem::is_regular_file(status))
    {
        return false;
    }

    pathIndex = getPathIndex(filePath);
    return true;
}

GeometryPaths::PathIndex
GeometryPaths::getPathIndex(const std::filesystem::path& path)
{
    auto [it, inserted] = m_pathMap.try_emplace(path, static_cast<PathIndex>(m_paths.size()));
    if (inserted)
        m_paths.push_back(path);
    return it->second;
}

bool
GeometryPaths::getInfo(GeometryHandle handle, GeometryInfo& info) const
{
    const auto index = static_cast<std::size_t>(handle);
    if (index >= m_info.size())
        return false;

    const auto& item = m_info[index];
    info.path = m_paths[static_cast<std::size_t>(item.pathIndex)];
    info.directory = m_paths[static_cast<std::size_t>(item.directoryPathIndex)];
    info.center = item.center;
    info.isNormalized = item.isNormalized;
    return true;
}

} // end namespace celestia::engine
