#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

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

std::string
readSourceFile(std::string_view relativePath)
{
    std::ifstream input(repoPath(relativePath));
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

TEST_SUITE_BEGIN("MVC Step5 frontend contract");

TEST_CASE("SDL frontend parses runtime view selection")
{
    const auto sdlMain = readSourceFile("src/celestia/sdl/sdlmain.cpp");

    CHECK(contains(sdlMain, "<celruntime/runtimeconfig.h>"));
    CHECK(contains(sdlMain, "celestia::runtime::RuntimeConfig runtimeConfig"));
    CHECK(contains(sdlMain, "\"--view=\""));
    CHECK(contains(sdlMain, "runtimeConfig.setSelectedViewId"));
    CHECK(contains(sdlMain, "window->run(settings, runtimeConfig)"));
}

TEST_CASE("SDL AppWindow initializes the selected runtime view")
{
    const auto appWindowHeader = readSourceFile("src/celestia/sdl/appwindow.h");
    CHECK(contains(appWindowHeader, "class RuntimeConfig"));
    CHECK(contains(appWindowHeader, "bool run(const Settings&, const celestia::runtime::RuntimeConfig&);"));

    const auto appWindowSource = readSourceFile("src/celestia/sdl/appwindow.cpp");
    CHECK(contains(appWindowSource, "initRuntimeView(runtimeConfig)"));
    CHECK(contains(appWindowSource, "getActiveViewRuntime()"));
    CHECK(contains(appWindowSource, "MVC Step5 legacy renderer bridge"));
}

TEST_SUITE_END();
