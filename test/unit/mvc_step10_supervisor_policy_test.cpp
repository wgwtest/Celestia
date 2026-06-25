#include <doctest.h>

#include <filesystem>
#include <fstream>
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

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step10 supervisor policy");

TEST_CASE("RuntimeAssemblyRunner writes a trace with transport and exit codes")
{
    auto tracePath = std::filesystem::temp_directory_path() /
        "celestia-step10-supervisor-trace.log";
    std::filesystem::remove(tracePath);

    celestia::runtime::RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(celestia::runtime::RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(std::string(celestia::runtime::RuntimeConfig::Debug2DViewId));
    runtimeConfig.setHostTransport("stdio-pipe");
    runtimeConfig.setDurationMilliseconds(120);

    auto config = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        "step10-trace-test");
    config.session.tickMilliseconds = 10;
    config.supervisor.traceFile = tracePath;
    config.supervisor.restartPolicy = "never";
    config.supervisor.maxRestarts = 0;

    celestia::runtime::assembly::RuntimeAssemblyRunner runner(config);
    const auto result = runner.run();
    CAPTURE(result.log);
    CHECK(result.success);

    {
        std::ifstream input(tracePath);
        REQUIRE(input.good());
        const std::string trace((std::istreambuf_iterator<char>(input)),
                                std::istreambuf_iterator<char>());
        CHECK(contains(trace, "sessionId=step10-trace-test"));
        CHECK(contains(trace, "transport=stdio-pipe"));
        CHECK(contains(trace, "controller exitCode=0"));
        CHECK(contains(trace, "model exitCode=0"));
        CHECK(contains(trace, "view exitCode=0"));
    }

    std::filesystem::remove(tracePath);
}

TEST_SUITE_END();
