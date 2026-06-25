#include <doctest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <celruntime/protocol/envelope.h>

TEST_SUITE_BEGIN("MVC Step6 protocol");

TEST_CASE("RuntimeEnvelope serializes stable protocol metadata")
{
    using celestia::runtime::protocol::RuntimeMessageKind;
    using celestia::runtime::protocol::RuntimeRole;

    celestia::runtime::protocol::RuntimeEnvelope envelope;
    envelope.sessionId = "session-42";
    envelope.sequenceId = 7;
    envelope.timestampUs = 123456;
    envelope.sourceRole = RuntimeRole::Controller;
    envelope.targetRole = RuntimeRole::Model;
    envelope.kind = RuntimeMessageKind::Command;
    envelope.name = "model.step";
    envelope.payload = "dt=0.016";

    CHECK(envelope.protocolVersion == celestia::runtime::protocol::CurrentProtocolVersion);

    const auto serialized = celestia::runtime::protocol::serializeEnvelope(envelope);
    CHECK(serialized.find("protocolVersion=1\n") != std::string::npos);
    CHECK(serialized.find("sessionId=session-42\n") != std::string::npos);
    CHECK(serialized.find("sequenceId=7\n") != std::string::npos);
    CHECK(serialized.find("timestampUs=123456\n") != std::string::npos);
    CHECK(serialized.find("sourceRole=controller\n") != std::string::npos);
    CHECK(serialized.find("targetRole=model\n") != std::string::npos);
    CHECK(serialized.find("kind=command\n") != std::string::npos);
    CHECK(serialized.find("name=model.step\n") != std::string::npos);
    CHECK(serialized.find("payload=dt%3D0.016\n") != std::string::npos);

    const auto parsed = celestia::runtime::protocol::deserializeEnvelope(serialized);
    REQUIRE(parsed.has_value());
    CHECK(parsed->protocolVersion == celestia::runtime::protocol::CurrentProtocolVersion);
    CHECK(parsed->sessionId == "session-42");
    CHECK(parsed->sequenceId == 7);
    CHECK(parsed->timestampUs == 123456);
    CHECK(parsed->sourceRole == RuntimeRole::Controller);
    CHECK(parsed->targetRole == RuntimeRole::Model);
    CHECK(parsed->kind == RuntimeMessageKind::Command);
    CHECK(parsed->name == "model.step");
    CHECK(parsed->payload == "dt=0.016");
}

TEST_CASE("RuntimeEnvelope rejects unknown protocol versions")
{
    const std::string serialized =
        "protocolVersion=999\n"
        "sessionId=session-42\n"
        "sequenceId=7\n"
        "timestampUs=123456\n"
        "sourceRole=controller\n"
        "targetRole=model\n"
        "kind=command\n"
        "name=model.step\n"
        "payload=dt%3D0.016\n";

    CHECK_FALSE(celestia::runtime::protocol::deserializeEnvelope(serialized).has_value());
}

TEST_CASE("Lifecycle factories create role-directed messages")
{
    using celestia::runtime::protocol::RuntimeMessageKind;
    using celestia::runtime::protocol::RuntimeRole;

    const auto hello = celestia::runtime::protocol::hello(RuntimeRole::Launcher, RuntimeRole::Model);
    CHECK(hello.protocolVersion == celestia::runtime::protocol::CurrentProtocolVersion);
    CHECK(hello.sourceRole == RuntimeRole::Launcher);
    CHECK(hello.targetRole == RuntimeRole::Model);
    CHECK(hello.kind == RuntimeMessageKind::Lifecycle);
    CHECK(hello.name == "runtime.hello");

    const auto ready = celestia::runtime::protocol::ready(RuntimeRole::Model, RuntimeRole::Launcher);
    CHECK(ready.sourceRole == RuntimeRole::Model);
    CHECK(ready.targetRole == RuntimeRole::Launcher);
    CHECK(ready.kind == RuntimeMessageKind::Lifecycle);
    CHECK(ready.name == "runtime.ready");

    const auto shutdown = celestia::runtime::protocol::shutdown(RuntimeRole::Launcher,
                                                               RuntimeRole::Broadcast);
    CHECK(shutdown.sourceRole == RuntimeRole::Launcher);
    CHECK(shutdown.targetRole == RuntimeRole::Broadcast);
    CHECK(shutdown.kind == RuntimeMessageKind::Lifecycle);
    CHECK(shutdown.name == "runtime.shutdown");
}

TEST_CASE("Runtime roles and message kinds reject unknown names")
{
    using celestia::runtime::protocol::RuntimeMessageKind;
    using celestia::runtime::protocol::RuntimeRole;

    CHECK(celestia::runtime::protocol::roleName(RuntimeRole::Launcher) == std::string_view("launcher"));
    CHECK(celestia::runtime::protocol::roleName(RuntimeRole::Model) == std::string_view("model"));
    CHECK(celestia::runtime::protocol::roleName(RuntimeRole::Controller) == std::string_view("controller"));
    CHECK(celestia::runtime::protocol::roleName(RuntimeRole::View) == std::string_view("view"));
    CHECK(celestia::runtime::protocol::roleName(RuntimeRole::Broadcast) == std::string_view("broadcast"));

    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::Lifecycle) == std::string_view("lifecycle"));
    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::Command) == std::string_view("command"));
    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::Event) == std::string_view("event"));
    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::ViewFrame) == std::string_view("viewFrame"));
    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::Heartbeat) == std::string_view("heartbeat"));
    CHECK(celestia::runtime::protocol::messageKindName(RuntimeMessageKind::Error) == std::string_view("error"));

    CHECK(celestia::runtime::protocol::roleFromString("controller") == RuntimeRole::Controller);
    CHECK(celestia::runtime::protocol::messageKindFromString("viewFrame") == RuntimeMessageKind::ViewFrame);
    CHECK_FALSE(celestia::runtime::protocol::roleFromString("renderer").has_value());
    CHECK_FALSE(celestia::runtime::protocol::messageKindFromString("snapshot").has_value());
}

TEST_SUITE_END();
