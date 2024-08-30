#include <cassert>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>

#define TEST_F(tester, name) \
    void func_##name()
#define RUN_TEST(tester, name)      \
    do { \
        std::cout << "Start test " << #name << "..." << std::endl;  \
        std::clock_t start = std::clock();  \
        func_##name();  \
        std::cout << "Finish test " << #name << ": " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;  \
    } while(false)
#define EXPECT_EQ(a, b) assert(a == b)
#define EXPECT_TRUE(a) assert(a)
#define EXPECT_FALSE(a) assert(!(a))
