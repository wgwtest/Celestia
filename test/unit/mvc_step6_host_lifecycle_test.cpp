#include <doctest.h>

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/transport/framedmessage.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
repoPath(std::string_view relativePath)
{
    return sourceRoot() / std::filesystem::path(relativePath);
}

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::string
tempName(std::string_view target, std::string_view suffix)
{
    const auto buildId = std::hash<std::string>{}(buildRoot().string());
    return "celestia-" + std::string(target) + "-" + std::to_string(buildId) +
        "-serve." + std::string(suffix);
}

std::filesystem::path
hostPath(std::string_view target)
{
#ifdef _WIN32
    return buildRoot() / "src" / "celruntime" / (std::string(target) + ".exe");
#else
    return buildRoot() / "src" / "celruntime" / std::string(target);
#endif
}

std::string
readText(const std::filesystem::path& path)
{
    std::ifstream input(path);
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string
readBinary(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void
writeText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path);
    REQUIRE(output.good());
    output << text;
}

void
writeBinary(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << text;
}

std::string
quotePath(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

celestia::runtime::protocol::RuntimeEnvelope
lifecycle(std::string name)
{
    auto envelope = celestia::runtime::protocol::lifecycle(
        celestia::runtime::protocol::RuntimeRole::Launcher,
        celestia::runtime::protocol::RuntimeRole::Model,
        std::move(name));
    envelope.sessionId = "host-lifecycle-test";
    return envelope;
}

std::vector<celestia::runtime::protocol::RuntimeEnvelope>
decodeFrames(std::string_view frames)
{
    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(frames);

    std::vector<celestia::runtime::protocol::RuntimeEnvelope> messages;
    for (;;)
    {
        auto result = reader.receive();
        if (result.status == celestia::runtime::transport::ReceiveStatus::WouldBlock)
            return messages;
        REQUIRE(result.status == celestia::runtime::transport::ReceiveStatus::Message);
        REQUIRE(result.message.has_value());
        messages.push_back(*result.message);
    }
}

void
writeHostServeScript(const std::filesystem::path& scriptPath,
                     const std::filesystem::path& exe,
                     const std::filesystem::path& inputPath,
                     const std::filesystem::path& outputPath)
{
    writeText(scriptPath,
              "@echo off\n" +
                  quotePath(exe) +
                  " --stdio --protocol-version=1 --view=celestia.view2d.debug --serve --session=host-lifecycle-test < " +
                  quotePath(inputPath) + " > " + quotePath(outputPath) + "\n" +
                  "exit /b %ERRORLEVEL%\n");
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step6 host lifecycle");

TEST_CASE("runtime CMake declares long-running host loop sources")
{
    const auto cmake = readText(repoPath("src/celruntime/CMakeLists.txt"));

    CHECK(cmake.find("process/runtimehostloop.cpp") != std::string::npos);
    CHECK(cmake.find("process/runtimehostloop.h") != std::string::npos);
}

TEST_CASE("process host serve mode handles lifecycle frames until shutdown")
{
    const auto exe = hostPath("celestia-model-host");
    CAPTURE(exe.string());
    REQUIRE(std::filesystem::exists(exe));

    const auto inputPath = std::filesystem::temp_directory_path() / tempName("celestia-model-host", "in");
    const auto outputPath = std::filesystem::temp_directory_path() / tempName("celestia-model-host", "out");
    const auto scriptPath = std::filesystem::temp_directory_path() / tempName("celestia-model-host", "cmd");

    const auto input =
        celestia::runtime::transport::encodeFrame(lifecycle(celestia::runtime::protocol::RuntimeHello)) +
        celestia::runtime::transport::encodeFrame(lifecycle(celestia::runtime::protocol::RuntimeHeartbeat)) +
        celestia::runtime::transport::encodeFrame(lifecycle(celestia::runtime::protocol::RuntimeShutdown));

    writeBinary(inputPath, input);
    std::filesystem::remove(outputPath);
    writeHostServeScript(scriptPath, exe, inputPath, outputPath);

    const auto command = "cmd.exe /d /c call " + quotePath(scriptPath);
    CHECK(std::system(command.c_str()) == 0);

    const auto messages = decodeFrames(readBinary(outputPath));
    REQUIRE(messages.size() == 3);

    CHECK(messages[0].name == celestia::runtime::protocol::RuntimeReady);
    CHECK(messages[0].sourceRole == celestia::runtime::protocol::RuntimeRole::Model);
    CHECK(messages[0].targetRole == celestia::runtime::protocol::RuntimeRole::Launcher);

    CHECK(messages[1].name == celestia::runtime::protocol::RuntimeHeartbeat);
    CHECK(messages[1].sourceRole == celestia::runtime::protocol::RuntimeRole::Model);
    CHECK(messages[1].targetRole == celestia::runtime::protocol::RuntimeRole::Launcher);

    CHECK(messages[2].name == celestia::runtime::protocol::RuntimeStopped);
    CHECK(messages[2].sourceRole == celestia::runtime::protocol::RuntimeRole::Model);
    CHECK(messages[2].targetRole == celestia::runtime::protocol::RuntimeRole::Launcher);
}

TEST_SUITE_END();
