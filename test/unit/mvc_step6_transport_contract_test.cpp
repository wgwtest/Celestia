#include <doctest.h>

#include <sstream>
#include <string>

#include <celruntime/protocol/envelope.h>
#include <celruntime/transport/framedmessage.h>
#include <celruntime/transport/stdiotransport.h>

namespace
{

celestia::runtime::protocol::RuntimeEnvelope
makeEnvelope(std::string name, std::uint64_t sequenceId)
{
    celestia::runtime::protocol::RuntimeEnvelope envelope;
    envelope.sessionId = "transport-session";
    envelope.sequenceId = sequenceId;
    envelope.timestampUs = 1000 + static_cast<std::int64_t>(sequenceId);
    envelope.sourceRole = celestia::runtime::protocol::RuntimeRole::Launcher;
    envelope.targetRole = celestia::runtime::protocol::RuntimeRole::Model;
    envelope.kind = celestia::runtime::protocol::RuntimeMessageKind::Lifecycle;
    envelope.name = std::move(name);
    return envelope;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step6 transport");

TEST_CASE("framed message encodes envelopes with content length")
{
    const auto envelope = makeEnvelope("runtime.hello", 1);
    const auto frame = celestia::runtime::transport::encodeFrame(envelope);
    const auto body = celestia::runtime::protocol::serializeEnvelope(envelope);

    CHECK(frame.rfind("Content-Length: ", 0) == 0);
    CHECK(frame.find("\n\n") != std::string::npos);
    CHECK(frame.find(body) != std::string::npos);

    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(frame);
    const auto result = reader.receive();
    REQUIRE(result.status == celestia::runtime::transport::ReceiveStatus::Message);
    REQUIRE(result.message.has_value());
    CHECK(result.message->name == "runtime.hello");
    CHECK(result.message->sequenceId == 1);
}

TEST_CASE("framed reader preserves adjacent message order")
{
    const auto first = makeEnvelope("runtime.hello", 1);
    const auto second = makeEnvelope("runtime.start", 2);

    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(celestia::runtime::transport::encodeFrame(first) +
                  celestia::runtime::transport::encodeFrame(second));

    const auto firstResult = reader.receive();
    REQUIRE(firstResult.status == celestia::runtime::transport::ReceiveStatus::Message);
    REQUIRE(firstResult.message.has_value());
    CHECK(firstResult.message->name == "runtime.hello");
    CHECK(firstResult.message->sequenceId == 1);

    const auto secondResult = reader.receive();
    REQUIRE(secondResult.status == celestia::runtime::transport::ReceiveStatus::Message);
    REQUIRE(secondResult.message.has_value());
    CHECK(secondResult.message->name == "runtime.start");
    CHECK(secondResult.message->sequenceId == 2);

    CHECK(reader.receive().status == celestia::runtime::transport::ReceiveStatus::WouldBlock);
}

TEST_CASE("framed reader distinguishes incomplete and malformed frames")
{
    const auto frame = celestia::runtime::transport::encodeFrame(makeEnvelope("runtime.hello", 1));

    celestia::runtime::transport::FramedMessageReader incomplete;
    incomplete.append(frame.substr(0, frame.find("\n\n") + 1));
    CHECK(incomplete.receive().status == celestia::runtime::transport::ReceiveStatus::WouldBlock);

    celestia::runtime::transport::FramedMessageReader malformed;
    malformed.append("Content-Length: bad\n\npayload");
    CHECK(malformed.receive().status == celestia::runtime::transport::ReceiveStatus::Malformed);
}

TEST_CASE("stdio transport sends and receives framed envelopes")
{
    const auto inputEnvelope = makeEnvelope("runtime.hello", 1);
    std::istringstream input(celestia::runtime::transport::encodeFrame(inputEnvelope));
    std::ostringstream output;

    celestia::runtime::transport::StdioTransport transport(input, output);
    const auto received = transport.receive();
    REQUIRE(received.status == celestia::runtime::transport::ReceiveStatus::Message);
    REQUIRE(received.message.has_value());
    CHECK(received.message->name == "runtime.hello");

    CHECK(transport.send(makeEnvelope("runtime.ready", 2)));

    celestia::runtime::transport::FramedMessageReader reader;
    reader.append(output.str());
    const auto sent = reader.receive();
    REQUIRE(sent.status == celestia::runtime::transport::ReceiveStatus::Message);
    REQUIRE(sent.message.has_value());
    CHECK(sent.message->name == "runtime.ready");
    CHECK(sent.message->sequenceId == 2);

    transport.close();
    CHECK(transport.receive().status == celestia::runtime::transport::ReceiveStatus::Closed);
}

TEST_SUITE_END();
