#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <celruntime/assembly/runtimeassemblyconfig.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/viewplugin/viewpluginregistry.h>

namespace
{

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step10 runtime assembly config");

TEST_CASE("RuntimeAssemblyConfig maps RuntimeConfig into explicit MVC assembly")
{
    celestia::runtime::RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(celestia::runtime::RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(std::string(celestia::runtime::RuntimeConfig::Debug2DViewId));
    runtimeConfig.setHostTransport("local-socket");
    runtimeConfig.setDurationMilliseconds(250);
    runtimeConfig.setPluginDirectory("plugins");

    const auto config = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        "step10-config-test");

    CHECK(config.session.id == "step10-config-test");
    CHECK(config.session.durationMilliseconds == 250);
    CHECK(config.resources.contentRoot == buildRoot());
    CHECK(config.transport.controlKind == "local-socket");
    CHECK(config.transport.dataKind == "framed-envelope");
    CHECK(config.model.hostName == "celestia-model-host");
    CHECK(config.controller.hostName == "celestia-controller-host");
    CHECK(config.view.id == std::string(celestia::runtime::RuntimeConfig::Debug2DViewId));
    CHECK(config.view.pluginDirectory == "plugins");
}

TEST_CASE("RuntimeAssemblyConfig validates resources, transport and registered view")
{
    auto config = celestia::runtime::assembly::RuntimeAssemblyConfig{};
    config.session.id = "step10-validation-test";
    config.resources.contentRoot = buildRoot();
    config.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    config.transport.controlKind = "stdio-pipe";
    config.transport.dataKind = "framed-envelope";
    config.view.id = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);

    const auto registry = celestia::runtime::viewplugin::builtinViewPluginRegistry();
    std::string error;
    CHECK(config.validate(registry, &error));

    config.view.id = "celestia.view.missing";
    CHECK_FALSE(config.validate(registry, &error));
    CHECK(error.find("unknown view id") != std::string::npos);

    config.view.id = std::string(celestia::runtime::RuntimeConfig::Debug2DViewId);
    config.transport.controlKind = "quantum-mailbox";
    CHECK_FALSE(config.validate(registry, &error));
    CHECK(error.find("unsupported transport") != std::string::npos);

    config.transport.controlKind = "stdio-pipe";
    config.resources.contentRoot.clear();
    CHECK_FALSE(config.validate(registry, &error));
    CHECK(error.find("contentRoot") != std::string::npos);
}

TEST_CASE("RuntimeAssemblyConfig loads a runtime config file")
{
    const auto configPath = std::filesystem::temp_directory_path() /
        "celestia-step10-runtime-config.yaml";
    {
        std::ofstream output(configPath);
        output <<
            "session:\n"
            "  id: step10-file-config\n"
            "  durationMs: 180\n"
            "  tickMs: 15\n"
            "resources:\n"
            "  contentRoot: .\n"
            "transport:\n"
            "  control:\n"
            "    kind: local-socket\n"
            "  data:\n"
            "    kind: shared-memory\n"
            "view:\n"
            "  id: celestia.view3d.opengl\n"
            "  pluginDir: plugins\n"
            "supervisor:\n"
            "  traceFile: step10.trace\n";
    }

    std::string error;
    const auto config = celestia::runtime::assembly::loadRuntimeAssemblyConfig(
        configPath,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        &error);

    REQUIRE_MESSAGE(config.has_value(), error);
    CHECK(config->session.id == "step10-file-config");
    CHECK(config->session.durationMilliseconds == 180);
    CHECK(config->session.tickMilliseconds == 15);
    CHECK(config->transport.controlKind == "local-socket");
    CHECK(config->transport.dataKind == "shared-memory");
    CHECK(config->view.id == "celestia.view3d.opengl");
    CHECK(config->view.pluginDirectory == "plugins");
    CHECK(config->supervisor.traceFile.filename() == "step10.trace");

    std::filesystem::remove(configPath);
}

TEST_CASE("RuntimeConfig accepts runtime config file arguments")
{
    celestia::runtime::RuntimeConfig runtimeConfig;

    CHECK(celestia::runtime::applyRuntimeConfigArgument(
        runtimeConfig, "--runtime-config=DOC/CODEX_DOC/examples/runtime-2d-local-socket.yaml"));
    CHECK(runtimeConfig.runtimeConfigPath() ==
          "DOC/CODEX_DOC/examples/runtime-2d-local-socket.yaml");
}

TEST_SUITE_END();
