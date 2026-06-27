#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/assembly/runtimeassemblyconfig.h>
#include <celruntime/assembly/runtimeassemblyrunner.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/protocol/lifecycle.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/process/processsupervisor.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/view3d/view3dhost.h>
#include <celruntime/view3d/view3dscene.h>

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

std::filesystem::path
contentRoot()
{
    return buildRoot() / "run-full";
}

std::string
readText(std::string_view relativePath)
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

celestia::runtime::protocol::SceneFrame
readSceneFrame(std::string_view relativePath)
{
    auto frame = celestia::runtime::protocol::deserializeSceneFrame(readText(relativePath));
    REQUIRE(frame.has_value());
    return *frame;
}

celestia::runtime::protocol::RuntimeEnvelope
lifecycle(std::string name)
{
    auto envelope = celestia::runtime::protocol::lifecycle(
        celestia::runtime::protocol::RuntimeRole::Launcher,
        celestia::runtime::protocol::RuntimeRole::View,
        std::move(name));
    envelope.sessionId = "step15-view3d-real-scene";
    return envelope;
}

celestia::runtime::protocol::RuntimeEnvelope
sceneFrameEnvelope(const celestia::runtime::protocol::SceneFrame& frame)
{
    return celestia::runtime::protocol::sceneFrameEnvelope(
        frame,
        celestia::runtime::protocol::RuntimeRole::Model,
        celestia::runtime::protocol::RuntimeRole::View);
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step15 View3D real scene consumption");

TEST_CASE("Step15 View3D scene state consumes scene.frame golden samples")
{
    const auto minimal = readSceneFrame("test/fixtures/mvc/scene_frame_vnext_minimal.sceneframe");
    const auto minimalState = celestia::runtime::view3d::buildView3DSceneState(minimal, {});

    CHECK(minimalState.sequence == 1);
    CHECK(minimalState.cameraFov == doctest::Approx(45.0));
    CHECK(minimalState.bodyCount == 1);
    CHECK(minimalState.starCount == 1);
    CHECK(minimalState.resourceCount == 1);

    const auto solarSystem = readSceneFrame("test/fixtures/mvc/scene_frame_vnext_solar_system.sceneframe");
    const auto solarState = celestia::runtime::view3d::buildView3DSceneState(solarSystem, {});

    CHECK(solarState.sequence == 7);
    CHECK(solarState.bodyCount == 3);
    CHECK(solarState.starCount == 1);
    CHECK(solarState.orbitCount == 1);
    CHECK(solarState.labelCount == 2);
}

TEST_CASE("Step15 View3D resource resolver resolves ResourceRef through content root")
{
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "stars.dat"));
    REQUIRE(std::filesystem::exists(contentRoot() / "data" / "solarsys.ssc"));

    celestia::runtime::protocol::SceneFrame frame;
    frame.sessionId = "step15-resource-resolution";
    frame.sequence = 2;
    frame.simulationTime = 2451545.0;
    frame.time.julianDayTdb = 2451545.0;
    frame.camera.fov = 45.0;
    frame.camera.nearPlane = 0.01;
    frame.camera.farPlane = 1.0e9;
    frame.observer.frame = "celestia:observer:universal";

    celestia::runtime::protocol::ResourceRef stars;
    stars.id = "res:catalog:stars";
    stars.kind = "catalog";
    stars.package = "celestia-core";
    stars.relativePath = "data/stars.dat";
    stars.required = true;
    frame.resources.push_back(stars);

    celestia::runtime::protocol::ResourceRef solarsys;
    solarsys.id = "res:catalog:solarsys";
    solarsys.kind = "catalog";
    solarsys.package = "celestia-core";
    solarsys.relativePath = "data/solarsys.ssc";
    frame.resources.push_back(solarsys);

    const auto state = celestia::runtime::view3d::buildView3DSceneState(frame, contentRoot());

    REQUIRE(state.resources.size() == 2);
    CHECK(state.resourceCount == 2);
    CHECK(state.resolvedResourceCount == 2);
    CHECK(state.missingRequiredResourceCount == 0);
    CHECK(state.resources.front().resolvedPath == contentRoot() / "data" / "stars.dat");
    CHECK(state.resources.front().exists);
}

TEST_CASE("Step15 View3D host reports real scene consumption details")
{
    auto frame = readSceneFrame("test/fixtures/mvc/scene_frame_vnext_solar_system.sceneframe");
    frame.resources.clear();

    celestia::runtime::protocol::ResourceRef stars;
    stars.id = "res:catalog:stars";
    stars.kind = "catalog";
    stars.package = "celestia-core";
    stars.relativePath = "data/stars.dat";
    stars.required = true;
    frame.resources.push_back(stars);

    celestia::runtime::view3d::View3DHostOptions options;
    options.contentRoot = contentRoot();
    celestia::runtime::view3d::View3DHost host("step15-view3d-real-scene", options);

    const auto started = host.handle(lifecycle(celestia::runtime::protocol::RuntimeStart));
    REQUIRE(started.size() == 1);
    CHECK(started.front().name == "view.ready3d");
    CHECK(contains(started.front().payload, "resources"));

    const auto rendered = host.handle(sceneFrameEnvelope(frame));
    REQUIRE(rendered.size() == 1);
    CHECK(rendered.front().name == "view.frameRendered");
    CHECK(contains(rendered.front().payload, "bodyCount=3"));
    CHECK(contains(rendered.front().payload, "starCount=1"));
    CHECK(contains(rendered.front().payload, "orbitCount=1"));
    CHECK(contains(rendered.front().payload, "resourceCount=1"));
    CHECK(contains(rendered.front().payload, "resolvedResourceCount=1"));
    CHECK(contains(rendered.front().payload, "cameraFov=35"));
}

TEST_CASE("Step15 runtime assembly routes real Model scene details to View3D over local-socket")
{
#ifndef _WIN32
    MESSAGE("Step15 live local-socket View3D route is Windows-only in this build");
    return;
#else
    celestia::runtime::RuntimeConfig runtimeConfig;
    runtimeConfig.setRuntimeMode(celestia::runtime::RuntimeMode::MultiProcess);
    runtimeConfig.setSelectedViewId(std::string(celestia::runtime::RuntimeConfig::DefaultViewId));
    runtimeConfig.setHostTransport("local-socket");
    runtimeConfig.setDurationMilliseconds(300);

    auto assembly = celestia::runtime::assembly::RuntimeAssemblyConfig::fromRuntimeConfig(
        runtimeConfig,
        buildRoot() / "src" / "celruntime",
        buildRoot(),
        "step15-real-view3d-runtime");
    assembly.session.tickMilliseconds = 30;
    assembly.session.readyTimeoutMilliseconds = 8000;
    assembly.session.shutdownTimeoutMilliseconds = 5000;

    celestia::runtime::assembly::RuntimeAssemblyRunner runner(std::move(assembly));
    const auto result = runner.run();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "transport=local-socket"));
    CHECK(contains(result.log, "model dataRoot="));
    CHECK(contains(result.log, "view contentRoot="));
    CHECK(contains(result.log, "scene.frame count="));
    CHECK(contains(result.log, "view.frameRendered count="));
    CHECK(contains(result.log, "bodyCount=1"));
    CHECK(contains(result.log, "starCount=1"));
    CHECK(contains(result.log, "resourceCount="));
    CHECK(contains(result.log, "resolvedResourceCount="));
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_CASE("Step15 ProcessSupervisor passes content root into real Model and View3D")
{
#ifndef _WIN32
    MESSAGE("Step15 live ProcessSupervisor View3D route is Windows-only in this build");
    return;
#else
    celestia::runtime::process::ProcessSupervisorOptions options;
    options.runtimeHostDirectory = buildRoot() / "src" / "celruntime";
    options.contentRoot = buildRoot();
    options.viewId = std::string(celestia::runtime::RuntimeConfig::DefaultViewId);
    options.durationMilliseconds = 300;
    options.hostTransport = "local-socket";
    options.sessionId = "step15-process-supervisor-real-content";

    celestia::runtime::process::ProcessSupervisor supervisor(options);
    const auto result = supervisor.runRuntime();

    CAPTURE(result.log);
    CHECK(result.success);
    CHECK(contains(result.log, "model dataRoot="));
    CHECK(contains(result.log, "view contentRoot="));
    CHECK(contains(result.log, "bodyCount=1"));
    CHECK(contains(result.log, "starCount=1"));
    CHECK(contains(result.log, "resolvedResourceCount="));
    CHECK(contains(result.log, "all hosts stopped"));
#endif
}

TEST_SUITE_END();
