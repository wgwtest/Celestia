#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <celruntime/model/modelsnapshot.h>
#include <celruntime/protocol/envelope.h>
#include <celruntime/view/viewservice.h>

namespace
{

using celestia::runtime::protocol::RuntimeEnvelope;
using celestia::runtime::protocol::RuntimeMessageKind;
using celestia::runtime::protocol::RuntimeRole;

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

RuntimeEnvelope
viewFrameEnvelope(std::uint64_t frameId, double time, std::string summary)
{
    celestia::runtime::ViewFrame frame;
    frame.frameId = frameId;
    frame.time = time;
    frame.summary = std::move(summary);

    RuntimeEnvelope envelope;
    envelope.sessionId = "view-service-test";
    envelope.sourceRole = RuntimeRole::Model;
    envelope.targetRole = RuntimeRole::View;
    envelope.kind = RuntimeMessageKind::ViewFrame;
    envelope.name = "view.frame";
    envelope.payload = celestia::runtime::model::serializeViewFrame(frame);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step6 view service");

TEST_CASE("ViewService consumes ViewFrame messages in a long-lived loop")
{
    celestia::runtime::view::ViewService service("view-service-test");
    CHECK_FALSE(service.isRunning());

    auto start = celestia::runtime::protocol::lifecycle(RuntimeRole::Launcher,
                                                       RuntimeRole::View,
                                                       "runtime.start");
    start.sessionId = "view-service-test";
    const auto started = service.handle(start);
    REQUIRE(started.size() == 1);
    CHECK(service.isRunning());
    CHECK(started.front().kind == RuntimeMessageKind::Lifecycle);
    CHECK(started.front().name == "view.running");

    const auto responses = service.handle(viewFrameEnvelope(7, 42.5, "synthetic frame"));
    CHECK(responses.empty());
    CHECK(service.lastFrameId() == 7);
    CHECK(service.lastSimulationTime() == doctest::Approx(42.5));
    CHECK(service.lastFrameSummary() == std::string_view("synthetic frame"));
    CHECK(service.frameCount() == 1);
}

TEST_CASE("ViewService emits close input without touching Controller objects")
{
    celestia::runtime::view::ViewService service("view-service-test");
    const auto close = service.closeRequested();

    CHECK(close.sourceRole == RuntimeRole::View);
    CHECK(close.targetRole == RuntimeRole::Controller);
    CHECK(close.kind == RuntimeMessageKind::Event);
    CHECK(close.name == "input.closeRequested");
}

TEST_CASE("ViewService remains headless and independent from 3D rendering")
{
    const auto viewHeader = readSourceFile("src/celruntime/view/viewservice.h");
    const auto viewSource = readSourceFile("src/celruntime/view/viewservice.cpp");

    CHECK_FALSE(contains(viewHeader, "celengine/view3d"));
    CHECK_FALSE(contains(viewSource, "celengine/view3d"));
    CHECK_FALSE(contains(viewHeader, "OpenGL"));
    CHECK_FALSE(contains(viewSource, "OpenGL"));
    CHECK_FALSE(contains(viewHeader, "glsupport"));
    CHECK_FALSE(contains(viewSource, "glsupport"));
}

TEST_SUITE_END();
