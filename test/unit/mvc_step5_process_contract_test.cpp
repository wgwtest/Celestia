#include <doctest.h>

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{

std::filesystem::path
sourceRoot()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path
repoPath(std::string_view relativePath)
{
    return sourceRoot() / std::filesystem::path(relativePath);
}

std::filesystem::path
buildRoot()
{
    return std::filesystem::current_path().parent_path().parent_path();
}

std::string
tempName(std::string_view target, std::string_view suffix)
{
    const auto buildId = std::hash<std::string>{}(buildRoot().string());
    return "celestia-" + std::string(target) + "-" + std::to_string(buildId) +
        "-handshake." + std::string(suffix);
}

std::filesystem::path
hostPath(std::string_view target)
{
#ifdef _WIN32
    return buildRoot() / "src" / "celruntime" / (std::string(target) + ".exe");
#else
    return buildRoot() / "src" / "celruntime" / std::string(target);
#endif
}

std::string
readText(const std::filesystem::path& path)
{
    std::ifstream input(path);
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void
writeText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path);
    REQUIRE(output.good());
    output << text;
}

std::string
quotePath(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

bool
contains(std::string_view text, std::string_view token)
{
    return text.find(token) != std::string_view::npos;
}

void
writeHostCommandScript(const std::filesystem::path& scriptPath,
                       const std::filesystem::path& exe,
                       const std::filesystem::path& inputPath,
                       const std::filesystem::path& outputPath)
{
    writeText(scriptPath,
              "@echo off\n" +
                  quotePath(exe) +
                  " --stdio --protocol-version=1 --view=celestia.view2d.debug --once < " +
                  quotePath(inputPath) + " > " + quotePath(outputPath) + "\n" +
                  "exit /b %ERRORLEVEL%\n");
}

void
runHostHandshake(std::string_view target, std::string_view role)
{
    const auto exe = hostPath(target);
    CAPTURE(exe.string());
    REQUIRE(std::filesystem::exists(exe));

    const auto inputPath = std::filesystem::temp_directory_path() / tempName(target, "in");
    const auto outputPath = std::filesystem::temp_directory_path() / tempName(target, "out");
    const auto scriptPath = std::filesystem::temp_directory_path() / tempName(target, "cmd");

    writeText(inputPath,
              "protocolVersion=1\n"
              "kind=command\n"
              "name=runtime.handshake\n"
              "payload=hello\n");
    std::filesystem::remove(outputPath);
    writeHostCommandScript(scriptPath, exe, inputPath, outputPath);

    const auto command = "cmd.exe /d /c call " + quotePath(scriptPath);

    const auto exitCode = std::system(command.c_str());
    CHECK(exitCode == 0);

    const auto output = readText(outputPath);
    CHECK(contains(output, "protocolVersion=1"));
    CHECK(contains(output, "kind=event"));
    CHECK(contains(output, "name=runtime.ack"));
    CHECK(contains(output, "payload=role%3D" + std::string(role)));
    CHECK(contains(output, "view%3Dcelestia.view2d.debug"));
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 process host contract");

TEST_CASE("runtime CMake declares process host executables")
{
    const auto cmake = readText(repoPath("src/celruntime/CMakeLists.txt"));

    CHECK(contains(cmake, "add_executable(celestia-model-host"));
    CHECK(contains(cmake, "add_executable(celestia-controller-host"));
    CHECK(contains(cmake, "add_executable(celestia-view-host"));
    CHECK(contains(cmake, "process/modelhostmain.cpp"));
    CHECK(contains(cmake, "process/controllerhostmain.cpp"));
    CHECK(contains(cmake, "process/viewhostmain.cpp"));
    CHECK(contains(cmake, "process/runtimehostcommon.cpp"));
}

TEST_CASE("process hosts support stdio once handshake")
{
    runHostHandshake("celestia-model-host", "model");
    runHostHandshake("celestia-controller-host", "controller");
    runHostHandshake("celestia-view-host", "view");
}

TEST_SUITE_END();
