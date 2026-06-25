#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <optional>
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

std::filesystem::path
repoPath(std::string_view relativePath)
{
    return sourceRoot() / std::filesystem::path(relativePath);
}

std::string
readText(std::string_view relativePath)
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

TEST_SUITE_BEGIN("MVC Step5 process mode selection");

TEST_CASE("RuntimeConfig exposes CLI process modes with stable defaults")
{
    celestia::runtime::RuntimeConfig config;

    CHECK(config.runtimeMode() == celestia::runtime::RuntimeMode::InProcessDirect);
    CHECK(config.selectedViewId() == "celestia.view3d.opengl");
    CHECK_FALSE(config.runOnce());

    CHECK(celestia::runtime::runtimeModeName(celestia::runtime::RuntimeMode::InProcessDirect) ==
          "in-process");
    CHECK(celestia::runtime::runtimeModeFromString("in-process") ==
          std::optional{ celestia::runtime::RuntimeMode::InProcessDirect });
    CHECK(celestia::runtime::runtimeModeFromString("multi-process") ==
          std::optional{ celestia::runtime::RuntimeMode::MultiProcess });
    CHECK_FALSE(celestia::runtime::runtimeModeFromString("remote-process").has_value());
}

TEST_CASE("RuntimeConfig applies SDL runtime arguments")
{
    celestia::runtime::RuntimeConfig config;

    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--view=celestia.view2d.debug"));
    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--mvc-mode=multi-process"));
    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--once"));

    CHECK(config.selectedViewId() == "celestia.view2d.debug");
    CHECK(config.runtimeMode() == celestia::runtime::RuntimeMode::MultiProcess);
    CHECK(config.runOnce());

    CHECK_FALSE(celestia::runtime::applyRuntimeConfigArgument(config, "--mvc-mode=bad-mode"));
}

TEST_CASE("SDL launcher routes multi-process debug2D through the MVC hosts")
{
    const auto sdlMain = readText("src/celestia/sdl/sdlmain.cpp");

    CHECK(contains(sdlMain, "--mvc-mode="));
    CHECK(contains(sdlMain, "applyRuntimeConfigArgument"));
    CHECK(contains(sdlMain, "RuntimeMode::MultiProcess"));
    CHECK(contains(sdlMain, "Debug2DViewId"));
    CHECK(contains(sdlMain, "celestia-model-host"));
    CHECK(contains(sdlMain, "celestia-controller-host"));
    CHECK(contains(sdlMain, "celestia-view-host"));
}

TEST_SUITE_END();
