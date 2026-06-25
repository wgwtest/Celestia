// hostprocess_win32.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "hostprocess.h"

#ifdef _WIN32

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <celruntime/transport/framedmessage.h>

namespace celestia::runtime::process
{
namespace
{

void
closeHandle(HANDLE& handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
    handle = nullptr;
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

std::wstring
quoteCommandLineArgument(std::wstring_view argument)
{
    if (argument.empty())
        return L"\"\"";

    const auto needsQuoting =
        argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needsQuoting)
        return std::wstring(argument);

    std::wstring quoted;
    quoted.push_back(L'"');

    std::size_t backslashes = 0;
    for (const auto ch : argument)
    {
        if (ch == L'\\')
        {
            ++backslashes;
            continue;
        }

        if (ch == L'"')
        {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashes = 0;
            continue;
        }

        if (backslashes > 0)
        {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
        }
        quoted.push_back(ch);
    }

    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

std::wstring
buildCommandLine(const HostProcessOptions& options)
{
    auto commandLine = quoteCommandLineArgument(options.executable.wstring());
    for (const auto& argument : options.arguments)
    {
        commandLine.push_back(L' ');
        commandLine += quoteCommandLineArgument(utf8ToWide(argument));
    }

    return commandLine;
}

DWORD
timeoutMilliseconds(std::chrono::milliseconds timeout)
{
    if (timeout.count() < 0)
        return 0;
    return static_cast<DWORD>(std::min<std::int64_t>(timeout.count(), INFINITE - 1));
}

} // end unnamed namespace

class HostProcess::Impl
{
public:
    explicit Impl(HostProcessOptions options)
        : options_(std::move(options))
    {
    }

    ~Impl()
    {
        if (running())
            terminate();

        closeHandle(childStdinWrite_);
        closeHandle(childStdoutRead_);
        closeHandle(process_);
    }

    bool start()
    {
        if (process_ != nullptr || options_.executable.empty())
            return false;

        SECURITY_ATTRIBUTES securityAttributes;
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.bInheritHandle = TRUE;
        securityAttributes.lpSecurityDescriptor = nullptr;

        HANDLE childStdoutWrite = nullptr;
        if (!CreatePipe(&childStdoutRead_, &childStdoutWrite, &securityAttributes, 0))
            return false;
        if (!SetHandleInformation(childStdoutRead_, HANDLE_FLAG_INHERIT, 0))
        {
            closeHandle(childStdoutRead_);
            closeHandle(childStdoutWrite);
            return false;
        }

        HANDLE childStdinRead = nullptr;
        if (!CreatePipe(&childStdinRead, &childStdinWrite_, &securityAttributes, 0))
        {
            closeHandle(childStdoutRead_);
            closeHandle(childStdoutWrite);
            return false;
        }
        if (!SetHandleInformation(childStdinWrite_, HANDLE_FLAG_INHERIT, 0))
        {
            closeHandle(childStdinRead);
            closeHandle(childStdinWrite_);
            closeHandle(childStdoutRead_);
            closeHandle(childStdoutWrite);
            return false;
        }

        STARTUPINFOW startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.hStdInput = childStdinRead;
        startupInfo.hStdOutput = childStdoutWrite;
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION processInfo;
        ZeroMemory(&processInfo, sizeof(processInfo));

        auto commandLine = buildCommandLine(options_);
        auto applicationName = options_.executable.wstring();
        auto workingDirectory = options_.workingDirectory.empty()
            ? std::wstring{}
            : options_.workingDirectory.wstring();

        const auto created = CreateProcessW(applicationName.c_str(),
                                            commandLine.data(),
                                            nullptr,
                                            nullptr,
                                            TRUE,
                                            CREATE_NO_WINDOW,
                                            nullptr,
                                            workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
                                            &startupInfo,
                                            &processInfo);

        closeHandle(childStdinRead);
        closeHandle(childStdoutWrite);

        if (!created)
        {
            closeHandle(childStdinWrite_);
            closeHandle(childStdoutRead_);
            return false;
        }

        process_ = processInfo.hProcess;
        closeHandle(processInfo.hThread);
        return true;
    }

    bool send(const protocol::RuntimeEnvelope& envelope)
    {
        if (childStdinWrite_ == nullptr || !running())
            return false;

        const auto frame = transport::encodeFrame(envelope);
        std::size_t offset = 0;
        while (offset < frame.size())
        {
            const auto remaining = frame.size() - offset;
            const auto chunk = static_cast<DWORD>(std::min<std::size_t>(remaining, 64 * 1024));
            DWORD written = 0;
            if (!WriteFile(childStdinWrite_, frame.data() + offset, chunk, &written, nullptr) ||
                written == 0)
            {
                return false;
            }
            offset += written;
        }

        return true;
    }

    std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        for (;;)
        {
            const auto parsed = reader_.receive();
            if (parsed.status == transport::ReceiveStatus::Message && parsed.message.has_value())
                return parsed.message;
            if (parsed.status == transport::ReceiveStatus::Malformed ||
                parsed.status == transport::ReceiveStatus::Closed)
            {
                return std::nullopt;
            }

            if (childStdoutRead_ == nullptr)
                return std::nullopt;

            DWORD available = 0;
            if (!PeekNamedPipe(childStdoutRead_, nullptr, 0, nullptr, &available, nullptr))
            {
                reader_.close();
                return std::nullopt;
            }

            if (available > 0)
            {
                std::string buffer(static_cast<std::size_t>(std::min<DWORD>(available, 4096)), '\0');
                DWORD read = 0;
                if (!ReadFile(childStdoutRead_, buffer.data(),
                              static_cast<DWORD>(buffer.size()), &read, nullptr) ||
                    read == 0)
                {
                    reader_.close();
                    return std::nullopt;
                }
                buffer.resize(read);
                reader_.append(buffer);
                continue;
            }

            if (!running())
            {
                reader_.close();
                return std::nullopt;
            }

            if (std::chrono::steady_clock::now() >= deadline)
                return std::nullopt;

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    std::optional<int> wait(std::chrono::milliseconds timeout)
    {
        if (process_ == nullptr)
            return std::nullopt;

        const auto waitResult = WaitForSingleObject(process_, timeoutMilliseconds(timeout));
        if (waitResult != WAIT_OBJECT_0)
            return std::nullopt;

        DWORD exitCode = 0;
        if (!GetExitCodeProcess(process_, &exitCode))
            return std::nullopt;

        return static_cast<int>(exitCode);
    }

    bool terminate()
    {
        if (process_ == nullptr)
            return false;

        if (running())
        {
            if (!TerminateProcess(process_, 1))
                return false;
            WaitForSingleObject(process_, 1000);
        }

        return true;
    }

    bool running() const
    {
        if (process_ == nullptr)
            return false;

        DWORD exitCode = 0;
        if (!GetExitCodeProcess(process_, &exitCode))
            return false;

        return exitCode == STILL_ACTIVE;
    }

private:
    HostProcessOptions options_;
    HANDLE process_{ nullptr };
    HANDLE childStdinWrite_{ nullptr };
    HANDLE childStdoutRead_{ nullptr };
    transport::FramedMessageReader reader_;
};

HostProcess::HostProcess(HostProcessOptions options)
    : impl_(std::make_unique<Impl>(std::move(options)))
{
}

HostProcess::~HostProcess() = default;

bool
HostProcess::start()
{
    return impl_->start();
}

bool
HostProcess::send(const protocol::RuntimeEnvelope& envelope)
{
    return impl_->send(envelope);
}

std::optional<protocol::RuntimeEnvelope>
HostProcess::receive(std::chrono::milliseconds timeout)
{
    return impl_->receive(timeout);
}

std::optional<int>
HostProcess::wait(std::chrono::milliseconds timeout)
{
    return impl_->wait(timeout);
}

bool
HostProcess::terminate()
{
    return impl_->terminate();
}

bool
HostProcess::running() const
{
    return impl_->running();
}

} // namespace celestia::runtime::process

#endif // _WIN32
