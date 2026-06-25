#include <doctest.h>

#include <filesystem>
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

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step9 view switch");

TEST_CASE("RuntimeConfig accepts view plugin listing and ordered switch arguments")
{
    celestia::runtime::RuntimeConfig config;

    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--list-views"));
    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--plugin-dir=build-mvc-sdl-rel/plugins"));
    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--switch-view-after-ms=2000"));
    CHECK(celestia::runtime::applyRuntimeConfigArgument(config, "--switch-view=celestia.view3d.opengl"));

    CHECK(config.listViews());
    CHECK(config.pluginDirectory() == "build-mvc-sdl-rel/plugins");
    CHECK(config.switchViewAfterMilliseconds() == 2000);
    CHECK(config.switchViewId() == "celestia.view3d.opengl");
}

TEST_CASE("RuntimeSession can stop debug2D and load OpenGL3D within the same MVC session")
{
#ifndef _WIN32
    MESSAGE("Step9 live RuntimeSession View switch is Windows-only in first implementation");
    return;
#else
    celestia::runtime::process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.switchViewAfterMilliseconds = 150;
    options.switchViewId = std::string(celestia::runtime::RuntimeConfig::DefaultViewId);
    options.sessionId = "step9-view-switch-test";
    options.durationMilliseconds = 450;
    options.tickMilliseconds = 50;
    options.hostReadyTimeoutMilliseconds = 3000;
    options.shutdownTimeoutMilliseconds = 3000;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "loaded view celestia.view2d.debug"));
    CHECK(contains(result.log, "stopped view celestia.view2d.debug"));
    CHECK(contains(result.log, "loaded view celestia.view3d.opengl"));
    CHECK(contains(result.log, "view.frame count="));
    CHECK(contains(result.log, "scene.frame count="));
#endif
}

TEST_SUITE_END();
