#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    // Remove all .log and .json files produced by tests
    for (fs::path path : fs::directory_iterator("."))
    {
        if (!fs::is_regular_file(path))
        {
            continue;
        }
        if (path.extension() != ".log" && path.extension() != ".json")
        {
            continue;
        }
        fs::remove(path);
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}