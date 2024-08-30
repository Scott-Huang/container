#include "../test/test.h"

#include <list>
#include <random>
#include "list.h"
#include "concurrent_list.h"

using namespace mem_container;

TEST_F(DefaultTester, Simple) {
    constexpr size_t N = 100;
    List<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_back(i);
        EXPECT_EQ(list.size(), i + 1);
        EXPECT_EQ(list.back(), i);
    }

    size_t i = 0;
    for (const auto &v : list) {
        EXPECT_EQ(v, i);
        ++i;
    }

    for (size_t i = 1; i < N; i++) {
        list.pop_back();
        EXPECT_EQ(list.size(), N - i);
        EXPECT_EQ(list.back(), N - i - 1);
    }
    list.pop_back();
    EXPECT_TRUE(list.empty());

    optional_destroy(list);
}

TEST_F(DefaultTester, Simple2) {
    constexpr size_t N = 100;
    List<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_front(i);
        EXPECT_EQ(list.size(), i + 1);
        EXPECT_EQ(list.front(), i);
    }

    size_t i = 0;
    for (const auto &v : list) {
        EXPECT_EQ(v, N - i - 1);
        ++i;
    }

    for (size_t i = 0; i < N - 1; i++) {
        list.pop_front();
        EXPECT_EQ(list.size(), N - i - 1);
        EXPECT_EQ(list.front(), N - i - 2);
    }
    list.pop_front();
    EXPECT_TRUE(list.empty());

    optional_destroy(list);
}

TEST_F(DefaultTester, DLSimple1) {
    constexpr size_t N = 100;
    DLList<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_back(i);
        EXPECT_EQ(list.size(), i + 1);
        EXPECT_EQ(list.back(), i);
    }

    size_t i = 0;
    for (const auto &v : list) {
        EXPECT_EQ(v, i);
        ++i;
    }

    for (size_t i = 1; i < N; i++) {
        list.pop_back();
        EXPECT_EQ(list.size(), N - i);
        EXPECT_EQ(list.back(), N - i - 1);
    }
    list.pop_back();
    EXPECT_TRUE(list.empty());

    optional_destroy(list);
}

TEST_F(DefaultTester, DLSimple2) {
    constexpr size_t N = 100;
    DLList<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_front(i);
        EXPECT_EQ(list.size(), i + 1);
        EXPECT_EQ(list.front(), i);
    }

    size_t i = 0;
    for (const auto &v : list) {
        EXPECT_EQ(v, N - i - 1);
        ++i;
    }

    for (size_t i = 0; i < N - 1; i++) {
        list.pop_front();
        EXPECT_EQ(list.size(), N - i - 1);
        EXPECT_EQ(list.front(), N - i - 2);
    }
    list.pop_front();
    EXPECT_TRUE(list.empty());

    optional_destroy(list);
}

TEST_F(DefaultTester, Benchmark) {
    constexpr size_t N = 100'000'000;
    List<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_back(i);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_front();
        list.push_back(i + N);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_front();
    }

    for (size_t i = 0; i < 1000; ++i) {
        List<size_t> llist;
        optional_destroy(llist);
    }

    optional_destroy(list);
}

TEST_F(DefaultTester, DLBenchmark) {
    constexpr size_t N = 100'000'000;
    DLList<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_back(i);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_back();
        list.push_back(i + N);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_back();
    }

    for (size_t i = 0; i < 1000; ++i) {
        DLList<size_t> llist;
        optional_destroy(llist);
    }

    optional_destroy(list);
}

TEST_F(DefaultTester, Reference) {
    constexpr size_t N = 100'000'000;
    std::list<size_t> list;
    for (size_t i = 0; i < N; i++) {
        list.push_back(i);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_back();
        list.push_back(i + N);
    }
    for (size_t i = 0; i < N; i++) {
        list.pop_back();
    }

    for (size_t i = 0; i < 1000; ++i) {
        std::list<size_t> llist;
    }
}

int main() {
    RUN_TEST(DefaultTester, Simple);
    RUN_TEST(DefaultTester, Simple2);
    RUN_TEST(DefaultTester, DLSimple1);
    RUN_TEST(DefaultTester, DLSimple2);
    RUN_TEST(DefaultTester, Benchmark);
    RUN_TEST(DefaultTester, DLBenchmark);
    RUN_TEST(DefaultTester, Reference);
    return 0;
}
