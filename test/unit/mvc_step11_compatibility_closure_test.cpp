#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/runtimeconfig.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::string
readSourceFile(std::string_view relativePath)
{
    std::ifstream input(sourceRoot() / std::filesystem::path(relativePath));
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step11 compatibility closure");

TEST_CASE("RuntimeConfig rejects the removed stdio-files migration fallback")
{
    celestia::runtime::RuntimeConfig config;

    CHECK_FALSE(celestia::runtime::applyRuntimeConfigArgument(config, "--host-transport=stdio-files"));
    CHECK(config.hostTransport() == "stdio-pipe");
}

TEST_CASE("Step6 file-script ProcessSupervisor fallback is removed from production sources")
{
    const auto runtimeConfig = readSourceFile("src/celruntime/runtimeconfig.cpp");
    const auto supervisorHeader = readSourceFile("src/celruntime/process/processsupervisor.h");
    const auto supervisorSource = readSourceFile("src/celruntime/process/processsupervisor.cpp");
    const auto sdlMain = readSourceFile("src/celestia/sdl/sdlmain.cpp");

    CHECK_FALSE(contains(runtimeConfig, "stdio-files"));
    CHECK_FALSE(contains(supervisorHeader, "runServeSmoke"));
    CHECK_FALSE(contains(supervisorSource, "runServeSmoke"));
    CHECK_FALSE(contains(sdlMain, "stdio-files"));
    CHECK_FALSE(contains(sdlMain, "sdl-step6-serve"));
}

TEST_SUITE_END();
