#include <doctest.h>

#include <optional>
#include <string>

#include <celruntime/ipc/localchannel.h>
#include <celruntime/ipc/message.h>
#include <celruntime/runtimecomposition.h>
#include <celruntime/runtimeconfig.h>
#include <celruntime/viewframe.h>

namespace
{

celestia::runtime::ViewFrame
makeFrame()
{
    celestia::runtime::ViewFrame frame;
    frame.time = 2451545.25;

    celestia::runtime::ViewFrameSelection earth;
    earth.type = "body";
    earth.id = "Earth";
    earth.positionKm = { 1.5, 2.5, 3.5 };
    earth.visible = true;
    earth.clickable = true;
    frame.selections.push_back(earth);

    celestia::runtime::ViewFrameSelection mars;
    mars.type = "body";
    mars.id = "Mars";
    mars.positionKm = { 4.5, 5.5, 6.5 };
    mars.visible = false;
    mars.clickable = true;
    frame.selections.push_back(mars);

    return frame;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 IPC contract");

TEST_CASE("message envelope is versioned and supports all runtime message kinds")
{
    using celestia::runtime::ipc::CurrentProtocolVersion;
    using celestia::runtime::ipc::MessageKind;
    using celestia::runtime::ipc::RuntimeMessage;
    using celestia::runtime::ipc::messageKindFromString;
    using celestia::runtime::ipc::messageKindName;

    auto command = RuntimeMessage::command("controller.goto", "target=Earth");
    CHECK(command.protocolVersion == CurrentProtocolVersion);
    CHECK(command.kind == MessageKind::Command);
    CHECK(command.name == "controller.goto");
    CHECK(command.payload == "target=Earth");

    CHECK(messageKindName(MessageKind::Command) == std::string_view("command"));
    CHECK(messageKindName(MessageKind::Event) == std::string_view("event"));
    CHECK(messageKindName(MessageKind::ViewFrame) == std::string_view("viewFrame"));
    CHECK(messageKindName(MessageKind::Error) == std::string_view("error"));

    auto commandKind = messageKindFromString("command");
    REQUIRE(commandKind.has_value());
    CHECK(*commandKind == MessageKind::Command);

    auto eventKind = messageKindFromString("event");
    REQUIRE(eventKind.has_value());
    CHECK(*eventKind == MessageKind::Event);

    auto viewFrameKind = messageKindFromString("viewFrame");
    REQUIRE(viewFrameKind.has_value());
    CHECK(*viewFrameKind == MessageKind::ViewFrame);

    auto errorKind = messageKindFromString("error");
    REQUIRE(errorKind.has_value());
    CHECK(*errorKind == MessageKind::Error);
    CHECK_FALSE(messageKindFromString("unknown").has_value());
}

TEST_CASE("unknown protocol versions are rejected during deserialization")
{
    const std::string serialized =
        "protocolVersion=999\n"
        "kind=command\n"
        "name=controller.goto\n"
        "payload=target=Earth\n";

    const auto parsed = celestia::runtime::ipc::deserializeMessage(serialized);
    CHECK_FALSE(parsed.has_value());
}

TEST_CASE("local channel preserves message order")
{
    celestia::runtime::ipc::LocalChannel channel;

    channel.send(celestia::runtime::ipc::RuntimeMessage::command("first", "1"));
    channel.send(celestia::runtime::ipc::RuntimeMessage::event("second", "2"));
    channel.send(celestia::runtime::ipc::RuntimeMessage::error("third", "3"));

    auto first = channel.receive();
    REQUIRE(first.has_value());
    CHECK(first->kind == celestia::runtime::ipc::MessageKind::Command);
    CHECK(first->name == "first");

    auto second = channel.receive();
    REQUIRE(second.has_value());
    CHECK(second->kind == celestia::runtime::ipc::MessageKind::Event);
    CHECK(second->name == "second");

    auto third = channel.receive();
    REQUIRE(third.has_value());
    CHECK(third->kind == celestia::runtime::ipc::MessageKind::Error);
    CHECK(third->name == "third");

    CHECK_FALSE(channel.receive().has_value());
}

TEST_CASE("message serialization roundtrips ViewFrame snapshots")
{
    const auto serialized = celestia::runtime::ipc::serializeMessage(
        celestia::runtime::ipc::RuntimeMessage::viewFrame("scene.frame", makeFrame()));

    auto parsed = celestia::runtime::ipc::deserializeMessage(serialized);
    REQUIRE(parsed.has_value());
    CHECK(parsed->protocolVersion == celestia::runtime::ipc::CurrentProtocolVersion);
    CHECK(parsed->kind == celestia::runtime::ipc::MessageKind::ViewFrame);
    CHECK(parsed->name == "scene.frame");
    CHECK(parsed->frame.time == doctest::Approx(2451545.25));
    REQUIRE(parsed->frame.selections.size() == 2);

    CHECK(parsed->frame.selections[0].type == "body");
    CHECK(parsed->frame.selections[0].id == "Earth");
    CHECK(parsed->frame.selections[0].positionKm[0] == doctest::Approx(1.5));
    CHECK(parsed->frame.selections[0].visible);
    CHECK(parsed->frame.selections[0].clickable);

    CHECK(parsed->frame.selections[1].id == "Mars");
    CHECK_FALSE(parsed->frame.selections[1].visible);
    CHECK(parsed->frame.selections[1].clickable);
}

TEST_CASE("RuntimeComposition can select direct or channel-backed in-process mode")
{
    celestia::runtime::RuntimeComposition direct;
    CHECK(direct.runtimeMode() == celestia::runtime::RuntimeMode::InProcessDirect);
    CHECK(direct.runtimeChannel() == nullptr);

    celestia::runtime::RuntimeConfig config;
    config.setRuntimeMode(celestia::runtime::RuntimeMode::InProcessChannel);

    celestia::runtime::RuntimeComposition channelBacked(config);
    CHECK(channelBacked.runtimeMode() == celestia::runtime::RuntimeMode::InProcessChannel);
    REQUIRE(channelBacked.runtimeChannel() != nullptr);

    channelBacked.runtimeChannel()->send(
        celestia::runtime::ipc::RuntimeMessage::event("runtime.ready", "ok"));

    auto message = channelBacked.runtimeChannel()->receive();
    REQUIRE(message.has_value());
    CHECK(message->name == "runtime.ready");
    CHECK(message->payload == "ok");
}

TEST_SUITE_END();
