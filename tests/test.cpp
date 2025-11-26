#include <gtest/gtest.h>
#include "SerializationTest.cpp"
#include "2702MathTest.cpp"

int main(int argc, char **argv){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
