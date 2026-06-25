#include <doctest.h>

#include <filesystem>
#include <string>
#include <string_view>

#include <celruntime/assembly/runtimeassemblyrunner.h>
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

celestia::runtime::assembly::RuntimeAssemblyConfig
assemblyConfig(std::string transportKind, std::string sessionId)
{
    celestia::runtime::RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(celestia::runtime::RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(std::string(celestia::runtime::RuntimeConfig::Debug2DViewId));
    runtimeConfig.setHostTransport(std::move(transportKind));
    runtimeConfig.setDurationMilliseconds(120);

    auto config = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        std::move(sessionId));
    config.session.tickMilliseconds = 10;
    return config;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step10 runtime assembly runner");

TEST_CASE("RuntimeAssemblyRunner starts the same 2D MVC session over stdio-pipe")
{
    celestia::runtime::assembly::RuntimeAssemblyRunner runner(
        assemblyConfig("stdio-pipe", "step10-runner-stdio"));
    const auto result = runner.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "assembly view=celestia.view2d.debug"));
    CHECK(contains(result.log, "transport=stdio-pipe"));
    CHECK(contains(result.log, "all hosts stopped"));
}

TEST_CASE("RuntimeAssemblyRunner starts the same 2D MVC session over local-socket")
{
#ifndef _WIN32
    MESSAGE("Step10 local-socket first implementation is Windows named-pipe based");
    return;
#else
    celestia::runtime::assembly::RuntimeAssemblyRunner runner(
        assemblyConfig("local-socket", "step10-runner-local-socket"));
    const auto result = runner.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "assembly view=celestia.view2d.debug"));
    CHECK(contains(result.log, "transport=local-socket"));
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_SUITE_END();
