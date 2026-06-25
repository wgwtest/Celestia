// sharedmemorydataplane.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "sharedmemorydataplane.h"

#include <cstring>
#include <map>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace celestia::runtime::dataplane
{
namespace
{

#ifdef _WIN32
void
closeHandle(HANDLE handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
}

std::wstring
utf8ToWide(std::string_view text)
{
    if (text.empty())
        return {};

    const auto size = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0)
        return std::wstring(text.begin(), text.end());

    std::wstring output(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        output.data(), size);
    return output;
}
#endif

} // end unnamed namespace

class SharedMemoryDataPlane::Impl
{
public:
    ~Impl()
    {
#ifdef _WIN32
        for (const auto& [id, handle] : mappings_)
        {
            (void)id;
            closeHandle(handle);
        }
#endif
    }

    DataPlaneRef publish(const std::vector<std::byte>& bytes, std::string_view label)
    {
        DataPlaneRef ref;
        ref.kind = "shared-memory";
        ref.id = makeId(label);
        ref.byteLength = bytes.size();
        ref.contentHash = dataPlaneContentHash(bytes);
        ref.transportHint = "shared-memory";

#ifdef _WIN32
        const auto name = utf8ToWide(ref.id);
        const auto highSize = static_cast<DWORD>((bytes.size() >> 32) & 0xffffffffu);
        const auto lowSize = static_cast<DWORD>(bytes.size() & 0xffffffffu);
        HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                            highSize, lowSize, name.c_str());
        if (mapping == nullptr)
            return {};

        void* view = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, bytes.size());
        if (view == nullptr)
        {
            closeHandle(mapping);
            return {};
        }

        std::memcpy(view, bytes.data(), bytes.size());
        UnmapViewOfFile(view);
        mappings_[ref.id] = mapping;
#else
        blocks_[ref.id] = bytes;
#endif
        return ref;
    }

    std::optional<std::vector<std::byte>> acquire(const DataPlaneRef& ref) const
    {
        if (ref.kind != "shared-memory")
            return std::nullopt;

#ifdef _WIN32
        const auto name = utf8ToWide(ref.id);
        HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, name.c_str());
        if (mapping == nullptr)
            return std::nullopt;

        const void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, ref.byteLength);
        if (view == nullptr)
        {
            closeHandle(mapping);
            return std::nullopt;
        }

        std::vector<std::byte> bytes(ref.byteLength);
        std::memcpy(bytes.data(), view, ref.byteLength);
        UnmapViewOfFile(view);
        closeHandle(mapping);
#else
        const auto iter = blocks_.find(ref.id);
        if (iter == blocks_.end())
            return std::nullopt;
        auto bytes = iter->second;
#endif
        if (dataPlaneContentHash(bytes) != ref.contentHash)
            return std::nullopt;
        return bytes;
    }

    bool release(const DataPlaneRef& ref)
    {
#ifdef _WIN32
        const auto iter = mappings_.find(ref.id);
        if (iter == mappings_.end())
            return false;
        closeHandle(iter->second);
        mappings_.erase(iter);
        return true;
#else
        return blocks_.erase(ref.id) > 0;
#endif
    }

private:
    std::string makeId(std::string_view label)
    {
        const auto id = "Local\\CelestiaMvcDataPlane-" + std::to_string(ownerId_) +
            "-" + std::string(label) + "-" + std::to_string(nextId_++);
        return id;
    }

#ifdef _WIN32
    std::map<std::string, HANDLE> mappings_;
    std::uint64_t ownerId_{ static_cast<std::uint64_t>(GetCurrentProcessId()) };
#else
    std::map<std::string, std::vector<std::byte>> blocks_;
    std::uint64_t ownerId_{ 0 };
#endif
    std::uint64_t nextId_{ 1 };
};

SharedMemoryDataPlane::SharedMemoryDataPlane()
    : impl_(std::make_unique<Impl>())
{
}

SharedMemoryDataPlane::~SharedMemoryDataPlane() = default;

DataPlaneRef
SharedMemoryDataPlane::publish(const std::vector<std::byte>& bytes, std::string_view label)
{
    return impl_->publish(bytes, label);
}

std::optional<std::vector<std::byte>>
SharedMemoryDataPlane::acquire(const DataPlaneRef& ref) const
{
    return impl_->acquire(ref);
}

bool
SharedMemoryDataPlane::release(const DataPlaneRef& ref)
{
    return impl_->release(ref);
}

} // namespace celestia::runtime::dataplane
