// localsockettransport.cpp
//
// Copyright (C) 2026, the Celestia Development Team

#include "localsockettransport.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include "framedmessage.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace celestia::runtime::transport
{
namespace
{

void
setError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

#ifdef _WIN32

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

std::string
makePipeName()
{
    static unsigned long counter = 1;
    return "\\\\.\\pipe\\celestia-mvc-" + std::to_string(GetCurrentProcessId()) +
        "-" + std::to_string(counter++);
}

class LocalPipeFramedTransport : public FramedTransport
{
public:
    explicit LocalPipeFramedTransport(HANDLE handle)
        : handle_(handle)
    {
    }

    ~LocalPipeFramedTransport() override
    {
        close();
    }

    bool send(const protocol::RuntimeEnvelope& envelope) override
    {
        if (closed_ || handle_ == nullptr || handle_ == INVALID_HANDLE_VALUE)
            return false;

        const auto frame = encodeFrame(envelope);
        std::size_t offset = 0;
        while (offset < frame.size())
        {
            const auto remaining = frame.size() - offset;
            const auto chunk = static_cast<DWORD>(std::min<std::size_t>(remaining, 64 * 1024));
            DWORD written = 0;
            if (!WriteFile(handle_, frame.data() + offset, chunk, &written, nullptr) ||
                written == 0)
            {
                reader_.close();
                return false;
            }
            offset += written;
        }
        sentBytes_ += frame.size();
        return true;
    }

    ReceiveResult receive() override
    {
        return receive(std::chrono::milliseconds::max());
    }

    ReceiveResult receive(std::chrono::milliseconds timeout)
    {
        if (closed_)
            return { ReceiveStatus::Closed, std::nullopt };

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        for (;;)
        {
            auto parsed = reader_.receive();
            if (parsed.status != ReceiveStatus::WouldBlock)
                return parsed;

            DWORD available = 0;
            if (!PeekNamedPipe(handle_, nullptr, 0, nullptr, &available, nullptr))
            {
                reader_.close();
                return reader_.receive();
            }

            if (available > 0)
            {
                std::string buffer(static_cast<std::size_t>(std::min<DWORD>(available, 4096)), '\0');
                DWORD read = 0;
                if (!ReadFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()),
                              &read, nullptr) ||
                    read == 0)
                {
                    reader_.close();
                    return reader_.receive();
                }
                buffer.resize(read);
                receivedBytes_ += read;
                reader_.append(buffer);
                continue;
            }

            if (timeout.count() != std::chrono::milliseconds::max().count() &&
                std::chrono::steady_clock::now() >= deadline)
            {
                return { ReceiveStatus::WouldBlock, std::nullopt };
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    void close() override
    {
        if (closed_)
            return;
        closed_ = true;
        reader_.close();
        closeHandle(handle_);
    }

    std::size_t sentBytes() const
    {
        return sentBytes_;
    }

    std::size_t receivedBytes() const
    {
        return receivedBytes_;
    }

private:
    HANDLE handle_{ nullptr };
    FramedMessageReader reader_;
    bool closed_{ false };
    std::size_t sentBytes_{ 0 };
    std::size_t receivedBytes_{ 0 };
};

bool
waitForClientConnection(HANDLE pipe, std::chrono::milliseconds timeout, std::string* error)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;)
    {
        if (ConnectNamedPipe(pipe, nullptr))
            return true;

        const auto code = GetLastError();
        if (code == ERROR_PIPE_CONNECTED)
            return true;
        if (code != ERROR_PIPE_LISTENING && code != ERROR_NO_DATA)
        {
            setError(error, "failed to connect local-socket pipe");
            return false;
        }

        if (std::chrono::steady_clock::now() >= deadline)
        {
            setError(error, "local-socket connection timeout");
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

#endif // _WIN32

} // end unnamed namespace

class LocalSocketTransport::Impl
{
public:
    explicit Impl(process::HostProcessOptions options)
        : options_(std::move(options))
    {
    }

    ~Impl()
    {
        close();
    }

    bool open(std::string* error)
    {
#ifndef _WIN32
        setError(error, "unsupported transport: local-socket");
        return false;
#else
        endpoint_ = makePipeName();
        auto pipe = CreateNamedPipeW(utf8ToWide(endpoint_).c_str(),
                                     PIPE_ACCESS_DUPLEX,
                                     PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
                                     1,
                                     64 * 1024,
                                     64 * 1024,
                                     0,
                                     nullptr);
        if (pipe == INVALID_HANDLE_VALUE)
        {
            setError(error, "failed to create local-socket pipe");
            return false;
        }

        auto processOptions = options_;
        processOptions.arguments.push_back("--local-socket=" + endpoint_);
        process_ = std::make_unique<process::HostProcess>(std::move(processOptions));
        if (!process_->start())
        {
            closeHandle(pipe);
            setError(error, "failed to start local-socket host");
            return false;
        }

        if (!waitForClientConnection(pipe, std::chrono::milliseconds(5000), error))
        {
            closeHandle(pipe);
            return false;
        }

        DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
        SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);
        pipeTransport_ = std::make_unique<LocalPipeFramedTransport>(pipe);
        return true;
#endif
    }

    bool send(const protocol::RuntimeEnvelope& envelope)
    {
        if (pipeTransport_ == nullptr || !pipeTransport_->send(envelope))
        {
            ++stats_.errors;
            return false;
        }
        ++stats_.sentMessages;
        stats_.sentBytes = pipeTransport_->sentBytes();
        return true;
    }

    std::optional<protocol::RuntimeEnvelope> receive(std::chrono::milliseconds timeout)
    {
        if (pipeTransport_ == nullptr)
            return std::nullopt;

        const auto received = pipeTransport_->receive(timeout);
        if (received.status == ReceiveStatus::Message && received.message.has_value())
        {
            ++stats_.receivedMessages;
            stats_.receivedBytes = pipeTransport_->receivedBytes();
            return received.message;
        }

        if (received.status != ReceiveStatus::WouldBlock)
            ++stats_.errors;
        return std::nullopt;
    }

    std::optional<int> wait(std::chrono::milliseconds timeout)
    {
        return process_ == nullptr ? std::nullopt : process_->wait(timeout);
    }

    bool terminate()
    {
        return process_ != nullptr && process_->terminate();
    }

    bool running() const
    {
        return process_ != nullptr && process_->running();
    }

    void close()
    {
        if (pipeTransport_ != nullptr)
            pipeTransport_->close();
        if (process_ != nullptr && process_->running())
            process_->terminate();
    }

    RuntimeTransportStats stats() const
    {
        return stats_;
    }

private:
    process::HostProcessOptions options_;
    std::unique_ptr<process::HostProcess> process_;
#ifdef _WIN32
    std::unique_ptr<LocalPipeFramedTransport> pipeTransport_;
#endif
    RuntimeTransportStats stats_;
    std::string endpoint_;
};

LocalSocketTransport::LocalSocketTransport(process::HostProcessOptions options)
    : impl_(std::make_unique<Impl>(std::move(options)))
{
}

LocalSocketTransport::~LocalSocketTransport() = default;

std::string_view
LocalSocketTransport::kind() const
{
    return "local-socket";
}

bool
LocalSocketTransport::open(std::string* error)
{
    return impl_->open(error);
}

bool
LocalSocketTransport::send(const protocol::RuntimeEnvelope& envelope)
{
    return impl_->send(envelope);
}

std::optional<protocol::RuntimeEnvelope>
LocalSocketTransport::receive(std::chrono::milliseconds timeout)
{
    return impl_->receive(timeout);
}

std::optional<int>
LocalSocketTransport::wait(std::chrono::milliseconds timeout)
{
    return impl_->wait(timeout);
}

bool
LocalSocketTransport::terminate()
{
    return impl_->terminate();
}

bool
LocalSocketTransport::running() const
{
    return impl_->running();
}

void
LocalSocketTransport::close()
{
    impl_->close();
}

RuntimeTransportStats
LocalSocketTransport::stats() const
{
    return impl_->stats();
}

bool
localSocketTransportSupported()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

std::unique_ptr<FramedTransport>
connectLocalSocketEndpoint(std::string_view endpoint, std::string* error)
{
#ifndef _WIN32
    setError(error, "unsupported transport: local-socket");
    return nullptr;
#else
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);
    HANDLE handle = INVALID_HANDLE_VALUE;
    for (;;)
    {
        handle = CreateFileW(utf8ToWide(endpoint).c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             nullptr,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);
        if (handle != INVALID_HANDLE_VALUE)
            break;

        const auto code = GetLastError();
        if (code != ERROR_PIPE_BUSY && code != ERROR_FILE_NOT_FOUND)
        {
            setError(error, "failed to open local-socket endpoint");
            return nullptr;
        }

        if (std::chrono::steady_clock::now() >= deadline)
        {
            setError(error, "local-socket endpoint timeout");
            return nullptr;
        }

        WaitNamedPipeW(utf8ToWide(endpoint).c_str(), 50);
    }

    DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
    SetNamedPipeHandleState(handle, &mode, nullptr, nullptr);
    return std::make_unique<LocalPipeFramedTransport>(handle);
#endif
}

} // namespace celestia::runtime::transport
