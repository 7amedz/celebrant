#include <gtest/gtest.h>

// Smoke test: proves the build/test toolchain works before any engine
// code exists. Replaced by real matching tests once the engine lands
TEST(Smoke, ToolchainWorks) {
    EXPECT_EQ(1 + 1, 2);
}
