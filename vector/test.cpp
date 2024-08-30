#include "../test/test.h"

#include <vector>
#include <random>
#include "vector.h"

using namespace mem_container;

TEST_F(DefaultTester, Simple) {
    constexpr size_t N = 100;
    Vector<size_t> vec;
    for (size_t i = 0; i < N; i++) {
        vec.push_back(i);
        EXPECT_EQ(vec.size(), i + 1);
        EXPECT_EQ(vec[i], i);
    }

    for (size_t i = 0; i < N; i++) {
        EXPECT_EQ(vec[i], i);
    }

    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
        EXPECT_EQ(vec.size(), N - i - 1);
    }

    optional_destroy(vec);
}

TEST_F(DefaultTester, Random) {
    constexpr size_t N = 100000;
    size_t *data = new size_t[N];
    for (size_t i = 0; i < N; i++) {
        data[i] = i;
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data, data + N, g);

    Vector<size_t> vec;
    for (size_t i = 0; i < N; i++) {
        vec.push_back(data[i]);
        EXPECT_EQ(vec.size(), i + 1);
        EXPECT_EQ(vec[i], data[i]);
    }

    for (size_t i = 0; i < N; i++) {
        EXPECT_EQ(vec[i], data[i]);
    }

    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
        EXPECT_EQ(vec.size(), N - i - 1);
    }

    optional_destroy(vec);
    delete[] data;
}

TEST_F(DefaultTester, Benchmark) {
    constexpr size_t N = 100'000'000;
    Vector<size_t, false, true> vec;
    for (size_t i = 0; i < N; i++) {
        vec.push_back(i);
    }
    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
        vec.push_back(i + N);
    }
    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
    }

    for (size_t i = 0; i < 1000; ++i) {
        Vector<size_t> vvec;
        optional_destroy(vvec);
    }

    optional_destroy(vec);
}

TEST_F(DefaultTester, Reference) {
    constexpr size_t N = 100'000'000;
    std::vector<size_t> vec;
    for (size_t i = 0; i < N; i++) {
        vec.push_back(i);
    }
    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
        vec.push_back(i + N);
    }
    for (size_t i = 0; i < N; i++) {
        vec.pop_back();
    }

    for (size_t i = 0; i < 1000; ++i) {
        std::vector<size_t> vvec;
    }

    optional_destroy(vec);
}

int main() {
    RUN_TEST(DefaultTester, Simple);
    RUN_TEST(DefaultTester, Random);
    RUN_TEST(DefaultTester, Benchmark);
    RUN_TEST(DefaultTester, Reference);
    return 0;
}
