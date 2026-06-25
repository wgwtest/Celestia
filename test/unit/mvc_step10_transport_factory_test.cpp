#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include <celruntime/process/hostprocess.h>
#include <celruntime/transport/localsockettransport.h>
#include <celruntime/transport/stdiopipetransport.h>
#include <celruntime/transport/transportfactory.h>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

celestia::runtime::process::HostProcessOptions
hostOptions()
{
    celestia::runtime::process::HostProcessOptions options;
    options.executable = std::filesystem::path("missing-host");
    options.workingDirectory = std::filesystem::current_path();
    options.arguments = {
        "--protocol-version=1",
        "--serve",
        "--session=step10-factory-test",
    };
    return options;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step10 transport factory");

TEST_CASE("transport factory creates stdio-pipe and local-socket transports")
{
    std::string error;
    auto stdio = celestia::runtime::transport::createRuntimeTransport(
        "stdio-pipe", hostOptions(), &error);
    REQUIRE(stdio != nullptr);
    CHECK(stdio->kind() == "stdio-pipe");

    auto local = celestia::runtime::transport::createRuntimeTransport(
        "local-socket", hostOptions(), &error);
    REQUIRE(local != nullptr);
    CHECK(local->kind() == "local-socket");
}

TEST_CASE("transport factory rejects unknown transport kinds with clear errors")
{
    std::string error;
    auto transport = celestia::runtime::transport::createRuntimeTransport(
        "tcp-socket", hostOptions(), &error);
    CHECK(transport == nullptr);
    CHECK(error.find("unsupported transport") != std::string::npos);
}

TEST_CASE("RuntimeSession source is transport-factory backed")
{
    const auto sourcePath = sourceRoot() /
        "src" / "celruntime" / "process" / "runtimesession.cpp";
    std::ifstream input(sourcePath);
    REQUIRE(input.good());
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());

    CHECK(source.find("createRuntimeTransport") != std::string::npos);
    CHECK(source.find("std::unique_ptr<RuntimeTransport>") != std::string::npos);
}

TEST_SUITE_END();
