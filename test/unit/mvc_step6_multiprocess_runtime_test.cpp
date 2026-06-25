#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/process/processsupervisor.h>
#include <celruntime/runtimeconfig.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
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

TEST_SUITE_BEGIN("MVC Step6 multi-process runtime");

TEST_CASE("ProcessSupervisor runs a three-host serve smoke")
{
    celestia::runtime::process::ProcessSupervisorOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.durationMilliseconds = 1000;
    options.hostTransport = "stdio";
    options.sessionId = "step6-supervisor-test";

    celestia::runtime::process::ProcessSupervisor supervisor(options);
    const auto result = supervisor.runServeSmoke();

    CHECK(result.success);
    CHECK(result.modelReady);
    CHECK(result.controllerReady);
    CHECK(result.viewReady);
    CHECK(result.viewFrameCount >= 4);
    CHECK(contains(result.log, "model ready"));
    CHECK(contains(result.log, "controller ready"));
    CHECK(contains(result.log, "view ready"));
    CHECK(contains(result.log, "all hosts stopped"));
}

TEST_CASE("SDL exposes Step6 multi-process serve runtime arguments")
{
    const auto runtimeHeader = readSourceFile("src/celruntime/runtimeconfig.h");
    const auto sdlMain = readSourceFile("src/celestia/sdl/sdlmain.cpp");
    const auto sdlCmake = readSourceFile("src/celestia/sdl/CMakeLists.txt");
    const auto runtimeCmake = readSourceFile("src/celruntime/CMakeLists.txt");

    CHECK(contains(runtimeHeader, "serve() const"));
    CHECK(contains(runtimeHeader, "durationMilliseconds() const"));
    CHECK(contains(runtimeHeader, "hostTransport() const"));
    CHECK(contains(sdlMain, "--serve"));
    CHECK(contains(sdlMain, "--duration-ms="));
    CHECK(contains(sdlMain, "--host-transport="));
    CHECK(contains(sdlMain, "ProcessSupervisor"));
    CHECK(contains(sdlCmake, "celestia-sdl-runtime-dependencies"));
    CHECK(contains(sdlCmake, "DEPENDS celestia"));
    CHECK(contains(sdlCmake, "add_dependencies(celestia-sdl celestia-sdl-runtime-dependencies)"));
    CHECK(contains(runtimeCmake, "process/processsupervisor.cpp"));
    CHECK(contains(runtimeCmake, "process/processsupervisor.h"));
}

TEST_SUITE_END();
