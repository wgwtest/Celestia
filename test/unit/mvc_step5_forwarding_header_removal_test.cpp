#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

std::string
readText(const std::filesystem::path& path)
{
    std::ifstream input(path);
    REQUIRE(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool
isSourceFile(const std::filesystem::path& path)
{
    const auto extension = path.extension().string();
    return extension == ".h" || extension == ".hpp" || extension == ".cpp" ||
           extension == ".cc" || extension == ".cxx" || extension == ".c";
}

std::vector<std::filesystem::path>
rootHeaders(std::string_view relativePath)
{
    std::vector<std::filesystem::path> headers;
    const auto root = repoPath(relativePath);
    if (!std::filesystem::exists(root))
        return headers;

    for (const auto& entry : std::filesystem::directory_iterator(root))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".h")
            headers.push_back(entry.path());
    }

    return headers;
}

bool
startsWith(std::string_view text, std::string_view prefix)
{
    return text.substr(0, prefix.size()) == prefix;
}

bool
containsSlash(std::string_view text)
{
    return text.find('/') != std::string_view::npos ||
           text.find('\\') != std::string_view::npos;
}

bool
isForwardingIncludePath(std::string_view includePath)
{
    if (startsWith(includePath, "celengine/"))
    {
        const auto ownedPath = includePath.substr(10);
        return ownedPath.find('/') == std::string_view::npos;
    }

    if (startsWith(includePath, "celrender/"))
    {
        const auto ownedPath = includePath.substr(10);
        return startsWith(ownedPath, "gl/") || ownedPath.find('/') == std::string_view::npos;
    }

    return false;
}

std::set<std::string>
ownedHeaderBasenames()
{
    std::set<std::string> basenames;

    constexpr std::string_view roots[] = {
        "src/celengine/model",
        "src/celengine/controller",
        "src/celengine/adapter",
        "src/celengine/view3d",
        "src/celengine/legacy",
        "src/celrender/view3d",
    };

    for (const auto root : roots)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(repoPath(root)))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".h")
                basenames.insert(entry.path().filename().string());
        }
    }

    return basenames;
}

std::vector<std::string>
forwardingIncludeViolations()
{
    std::vector<std::string> violations;
    const auto srcRoot = repoPath("src");
    const auto testRoot = repoPath("test");
    const auto ownedHeaders = ownedHeaderBasenames();

    for (const auto root : { srcRoot, testRoot })
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file() || !isSourceFile(entry.path()))
                continue;

            std::istringstream lines(readText(entry.path()));
            std::string line;
            int lineNumber = 0;
            while (std::getline(lines, line))
            {
                ++lineNumber;
                const auto angleIncludeStart = line.find("#include <");
                if (angleIncludeStart != std::string::npos)
                {
                    const auto pathStart = line.find('<', angleIncludeStart);
                    const auto pathEnd = line.find('>', pathStart);
                    if (pathStart == std::string::npos || pathEnd == std::string::npos)
                        continue;

                    const auto includePath = std::string_view(line).substr(pathStart + 1,
                                                                           pathEnd - pathStart - 1);
                    if (!isForwardingIncludePath(includePath))
                        continue;

                    violations.push_back(entry.path().lexically_relative(sourceRoot()).string() +
                                         ":" + std::to_string(lineNumber) + ": " + line);
                    continue;
                }

                const auto quoteIncludeStart = line.find("#include \"");
                if (quoteIncludeStart == std::string::npos)
                    continue;

                const auto pathStart = line.find('"', quoteIncludeStart);
                const auto pathEnd = line.find('"', pathStart + 1);
                if (pathStart == std::string::npos || pathEnd == std::string::npos)
                    continue;

                const auto includePath = std::string(line.substr(pathStart + 1,
                                                                 pathEnd - pathStart - 1));
                if (startsWith(includePath, "celengine/") || startsWith(includePath, "celrender/"))
                {
                    violations.push_back(entry.path().lexically_relative(sourceRoot()).string() +
                                         ":" + std::to_string(lineNumber) + ": " + line);
                    continue;
                }

                if (containsSlash(includePath))
                    continue;

                if (std::filesystem::exists(entry.path().parent_path() / includePath))
                    continue;

                if (ownedHeaders.find(includePath) == ownedHeaders.end())
                    continue;

                violations.push_back(entry.path().lexically_relative(sourceRoot()).string() +
                                     ":" + std::to_string(lineNumber) + ": " + line);
            }
        }
    }

    return violations;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("MVC Step5 forwarding header removal");

TEST_CASE("Step4 root forwarding header directories no longer expose public headers")
{
    CHECK(rootHeaders("src/celengine").empty());
    CHECK(rootHeaders("src/celrender").empty());
    CHECK(rootHeaders("src/celrender/gl").empty());
}

TEST_CASE("source includes real ownership paths instead of root forwarding headers")
{
    const auto violations = forwardingIncludeViolations();

    for (const auto& violation : violations)
    {
        CAPTURE(violation);
        CHECK(false);
    }

    CHECK(violations.empty());
}

TEST_SUITE_END();
