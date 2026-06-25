#include <doctest.h>

#include <vector>

#include <celruntime/dataplane/sharedmemorydataplane.h>

TEST_SUITE_BEGIN("MVC Step10 shared memory data plane");

TEST_CASE("SharedMemoryDataPlane transfers a byte block by DataPlaneRef")
{
#ifndef _WIN32
    MESSAGE("Step10 shared memory first implementation is Windows file-mapping based");
    return;
#else
    celestia::runtime::dataplane::SharedMemoryDataPlane producer;
    celestia::runtime::dataplane::SharedMemoryDataPlane consumer;
    const std::vector<std::byte> bytes{
        std::byte{ 0x10 },
        std::byte{ 0x20 },
        std::byte{ 0x30 },
        std::byte{ 0x40 },
    };

    const auto ref = producer.publish(bytes, "mesh.block");
    CHECK(ref.kind == "shared-memory");
    CHECK(ref.byteLength == bytes.size());
    CHECK_FALSE(ref.contentHash.empty());

    const auto acquired = consumer.acquire(ref);
    REQUIRE(acquired.has_value());
    CHECK(*acquired == bytes);

    CHECK(producer.release(ref));
#endif
}

TEST_SUITE_END();
