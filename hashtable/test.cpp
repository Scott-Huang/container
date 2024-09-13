#include "../test/test.h"

#include <random>
#include "hashtable.h"

using namespace mem_container;

constexpr uint SEED = 90231505u;

TEST_F(DefaultTester, Simple) {
    HashSet<int> ht;
    bool res;
    res = ht.insert(1, {});
    EXPECT_TRUE(res);
    res = ht.insert(5, {});
    EXPECT_TRUE(res);
    res = ht.insert(9, {});
    EXPECT_TRUE(res);
    res = ht.contains(1);
    EXPECT_TRUE(res);
    res = ht.contains(5);
    EXPECT_TRUE(res);
    res = ht.contains(9);
    EXPECT_TRUE(res);
    res = ht.contains(2);
    EXPECT_FALSE(res);
    res = ht.contains(6);
    EXPECT_FALSE(res);
    res = ht.contains(10);
    EXPECT_FALSE(res);

    ht.destroy();
}

TEST_F(DefaultTester, Large) {
    HashSet<int> ht(1000);
    bool res;
    for (int i = 0; i < 1000; ++i) {
        res = ht.insert(i, {});
        EXPECT_TRUE(res);
    }
    for (int i = 0; i < 1000; ++i) {
        res = ht.contains(i);
        EXPECT_TRUE(res);
    }
    for (int i = 1000; i < 2000; ++i) {
        res = ht.contains(i);
        EXPECT_FALSE(res);
    }
    for (int i = 0; i < 1000; ++i) {
        res = ht.insert(i, {});
        EXPECT_FALSE(res);
    }

    ht.destroy();
}

TEST_F(DefaultTester, Benchmark) {
    constexpr int N = 10'000'000;
    HashSet<int> ht(N);
    bool res;
    for (int i = 0; i < N; ++i) {
        ht.insert(i, {});
    }
    for (int i = 0; i < N; ++i) {
        res = ht.contains(i);
        EXPECT_TRUE(res);
    }
    for (int i = N; i < 2 * N; ++i) {
        res = ht.contains(i);
        EXPECT_FALSE(res);
    }

    ht.destroy();
}

#include <unordered_set>
TEST_F(DefaultTester, Reference) {
    constexpr int N = 10'000'000;
    std::unordered_set<int> ht(N);
    bool res;
    for (int i = 0; i < N; ++i) {
        ht.insert(i);
    }
    for (int i = 0; i < N; ++i) {
        res = ht.count(i) > 0;
        EXPECT_TRUE(res);
    }
    for (int i = N; i < 2 * N; ++i) {
        res = ht.count(i) > 0;
        EXPECT_FALSE(res);
    }
}

int main() {
    RUN_TEST(DefaultTester, Simple);
    RUN_TEST(DefaultTester, Large);
    RUN_TEST(DefaultTester, Benchmark);
    RUN_TEST(DefaultTester, Reference);
}
