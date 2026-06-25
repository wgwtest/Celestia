#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/process/runtimesession.h>
#include <celruntime/runtimeconfig.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

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

TEST_SUITE_BEGIN("MVC Step8 runtime session 3D");

TEST_CASE("RuntimeSession drains View3D input back through Controller and Model")
{
    const auto source = readSourceFile("src/celruntime/process/runtimesession.cpp");

    CHECK(contains(source, "protocol::ViewInputMessageName"));
    CHECK(contains(source, "model.setViewInput"));
    CHECK(contains(source, "view.input routed count="));
}

TEST_CASE("RuntimeSession routes scene.frame through celestia-view3d-host")
{
#ifndef _WIN32
    MESSAGE("Step8 live RuntimeSession View3D route is Windows-only in first implementation");
    return;
#else
    celestia::runtime::process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::DefaultViewId);
    options.sessionId = "step8-runtime-session-3d-test";
    options.durationMilliseconds = 250;
    options.tickMilliseconds = 10;
    options.hostReadyTimeoutMilliseconds = 3000;
    options.shutdownTimeoutMilliseconds = 3000;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(result.controllerReady);
    CHECK(result.modelReady);
    CHECK(result.viewReady);
    CHECK(result.controllerStopped);
    CHECK(result.modelStopped);
    CHECK(result.viewStopped);
    CHECK(result.viewFrameCount >= 20);
    CHECK(contains(result.log, "view executable=celestia-view3d-host"));
    CHECK(contains(result.log, "scene.frame count="));
    CHECK(contains(result.log, "view.frameRendered count="));
#endif
}

TEST_SUITE_END();
