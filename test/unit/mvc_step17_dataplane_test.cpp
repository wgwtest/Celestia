#include <doctest.h>

#include <cstddef>
#include <vector>

#include <celruntime/dataplane/inprocessdataplane.h>
#include <celruntime/dataplane/dataplaneref.h>
#include <celruntime/protocol/sceneprotocol.h>
#include <celruntime/view3d/view3dresources.h>
#include <celruntime/view3d/view3dscene.h>

TEST_SUITE_BEGIN("MVC Step17 DataPlane resource bridge");

TEST_CASE("Step17 ResourceRef dataPlaneKey carries a serialized DataPlaneRef")
{
    celestia::runtime::dataplane::InProcessDataPlane channel;
    const std::vector<std::byte> bytes{ std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 } };
    const auto ref = channel.publish(bytes, "mesh.block");

    celestia::runtime::protocol::ResourceRef resource;
    resource.id = "res:mesh:block";
    resource.kind = "mesh";
    resource.package = "celestia-core";
    resource.relativePath = "models/generated/block.mesh";
    resource.dataPlaneKey = celestia::runtime::dataplane::serializeDataPlaneRef(ref);
    resource.required = true;

    const auto parsed = celestia::runtime::view3d::dataPlaneRefFromResource(resource);
    REQUIRE(parsed.has_value());
    CHECK(parsed->kind == ref.kind);
    CHECK(parsed->id == ref.id);
    CHECK(parsed->byteLength == bytes.size());

    const auto acquired = channel.acquire(*parsed);
    REQUIRE(acquired.has_value());
    CHECK(*acquired == bytes);
}

TEST_CASE("Step17 ResourceRef resolver counts DataPlane eligible resources")
{
    celestia::runtime::dataplane::InProcessDataPlane channel;
    const std::vector<std::byte> bytes{ std::byte{ 4 }, std::byte{ 5 } };
    const auto ref = channel.publish(bytes, "orbit.block");

    celestia::runtime::protocol::SceneFrame frame;
    celestia::runtime::protocol::ResourceRef resource;
    resource.id = "res:orbit:block";
    resource.kind = "orbit-sample";
    resource.package = "celestia-core";
    resource.relativePath = "generated/orbits/earth.bin";
    resource.dataPlaneKey = celestia::runtime::dataplane::serializeDataPlaneRef(ref);
    frame.resources.push_back(resource);

    const auto state = celestia::runtime::view3d::buildView3DSceneState(frame, {});
    REQUIRE(state.resources.size() == 1);
    CHECK(state.dataPlaneEligibleResourceCount == 1);
    CHECK(state.resources.front().dataPlaneEligible);
    CHECK(state.resources.front().dataPlaneRef.has_value());
    CHECK(state.resources.front().cacheKey == resource.dataPlaneKey);
}

TEST_CASE("Step17 legacy dataPlaneKey remains a cache key without DataPlane eligibility")
{
    celestia::runtime::protocol::SceneFrame frame;
    celestia::runtime::protocol::ResourceRef resource;
    resource.id = "res:texture:legacy";
    resource.kind = "texture";
    resource.package = "celestia-core";
    resource.relativePath = "textures/legacy.png";
    resource.dataPlaneKey = "legacy-cache-key";
    frame.resources.push_back(resource);

    const auto state = celestia::runtime::view3d::buildView3DSceneState(frame, {});
    REQUIRE(state.resources.size() == 1);
    CHECK(state.dataPlaneEligibleResourceCount == 0);
    CHECK_FALSE(state.resources.front().dataPlaneEligible);
    CHECK_FALSE(state.resources.front().dataPlaneRef.has_value());
    CHECK(state.resources.front().cacheKey == "legacy-cache-key");
}

TEST_SUITE_END();
