#include <doctest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <celruntime/assembly/runtimeassemblyconfig.h>
#include <celruntime/process/runtimesession.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/viewplugin/viewpluginregistry.h>

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

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step11 runtime acceptance");

TEST_CASE("Runtime assembly examples load and validate against the View registry")
{
    const std::vector<std::string> examples = {
        "runtime-2d-stdio.yaml",
        "runtime-2d-local-socket.yaml",
        "runtime-3d-stdio.yaml",
        "runtime-3d-local-socket.yaml",
        "runtime-switch-2d-to-3d-local-socket.yaml",
        "runtime-switch-3d-to-2d-local-socket.yaml",
    };

    const auto registry = celestia::runtime::viewplugin::builtinViewPluginRegistry();
    for (const auto& example : examples)
    {
        CAPTURE(example);
        std::string error;
        const auto config = celestia::runtime::assembly::loadRuntimeAssemblyConfig(
            sourceRoot() / "DOC" / "CODEX_DOC" / "examples" / example,
            buildRoot() / "src" / "celruntime",
            sourceRoot(),
            &error);

        REQUIRE_MESSAGE(config.has_value(), error);
        CHECK(config->validate(registry, &error));
    }
}

TEST_CASE("RuntimeSession can stop OpenGL3D and load debug2D within the same MVC session")
{
#ifndef _WIN32
    MESSAGE("Step11 live RuntimeSession reverse View switch is Windows-only in first implementation");
    return;
#else
    celestia::runtime::process::RuntimeSessionOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.viewId = std::string(celestia::runtime::RuntimeConfig::DefaultViewId);
    options.hostTransport = "local-socket";
    options.switchViewAfterMilliseconds = 150;
    options.switchViewId = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    options.sessionId = "step11-view-switch-reverse-test";
    options.durationMilliseconds = 500;
    options.tickMilliseconds = 50;
    options.hostReadyTimeoutMilliseconds = 3000;
    options.shutdownTimeoutMilliseconds = 3000;

    celestia::runtime::process::RuntimeSession session(options);
    const auto result = session.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "loaded view celestia.view3d.opengl"));
    CHECK(contains(result.log, "stopped view celestia.view3d.opengl"));
    CHECK(contains(result.log, "loaded view celestia.view2d.debug"));
    CHECK(contains(result.log, "scene.frame count="));
    CHECK(contains(result.log, "view.frame count="));
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_SUITE_END();
