#include <doctest.h>

#include <string>
#include <vector>

#include <celruntime/dataplane/inprocessdataplane.h>
#include <celruntime/protocol/envelope.h>

TEST_SUITE_BEGIN("MVC Step10 data plane contract");

TEST_CASE("DataPlaneRef serializes into a RuntimeEnvelope payload")
{
    celestia::runtime::dataplane::DataPlaneRef ref;
    ref.kind = "in-process";
    ref.id = "scene-1";
    ref.byteLength = 4;
    ref.contentHash = "hash";
    ref.transportHint = "framed-envelope";

    celestia::runtime::protocol::RuntimeEnvelope envelope;
    envelope.name = "scene.delta";
    envelope.payload = celestia::runtime::dataplane::serializeDataPlaneRef(ref);

    const auto parsed = celestia::runtime::dataplane::deserializeDataPlaneRef(envelope.payload);
    REQUIRE(parsed.has_value());
    CHECK(parsed->kind == "in-process");
    CHECK(parsed->id == "scene-1");
    CHECK(parsed->byteLength == 4);
    CHECK(parsed->contentHash == "hash");
    CHECK(parsed->transportHint == "framed-envelope");
}

TEST_CASE("InProcessDataPlane publishes acquires and releases byte blocks")
{
    celestia::runtime::dataplane::InProcessDataPlane channel;
    const std::vector<std::byte> bytes{
        std::byte{ 0x01 },
        std::byte{ 0x02 },
        std::byte{ 0x03 },
    };

    const auto ref = channel.publish(bytes, "scene.frame");
    CHECK(ref.kind == "in-process");
    CHECK(ref.byteLength == bytes.size());
    CHECK_FALSE(ref.contentHash.empty());

    const auto acquired = channel.acquire(ref);
    REQUIRE(acquired.has_value());
    CHECK(*acquired == bytes);

    CHECK(channel.release(ref));
    CHECK_FALSE(channel.acquire(ref).has_value());
}

TEST_SUITE_END();
