#include "../test/test.h"

#include <random>
#include "interval_set.h"

using IntervalSetType = IntervalSet<size_t>;
#define IntervalSetContain(set, ival) (set.contains(ival))

constexpr uint SEED = 90231505u;

TEST_F(IntervalSetTest, Simple) {
    IntervalSetType is;
    is.insert(IntervalSetType::interval_type::right_open(1, 3));
    is.insert(IntervalSetType::interval_type::right_open(5, 7));
    is.insert(IntervalSetType::interval_type::right_open(9, 11));

    EXPECT_EQ(is.iterative_size(), 3);
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(1, 3)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(5, 7)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(9, 11)));
    EXPECT_FALSE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(0, 3)));
    EXPECT_FALSE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(6, 8)));
    EXPECT_FALSE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(8, 12)));
    optional_destroy(is);
}

TEST_F(IntervalSetTest, Overlap) {
    IntervalSetType is;
    is.insert(IntervalSetType::interval_type::right_open(1, 3));
    is.insert(IntervalSetType::interval_type::right_open(5, 7));
    is.insert(IntervalSetType::interval_type::right_open(9, 11));
    is.insert(IntervalSetType::interval_type::right_open(2, 6));
    is.insert(IntervalSetType::interval_type::right_open(4, 10));
    is.insert(IntervalSetType::interval_type::right_open(10, 12));

    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(2, 3)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(3, 7)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(5, 11)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(1, 12)));
    EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(9, 10)));
    optional_destroy(is);
}

TEST_F(IntervalSetTest, OverlapExplicit) {
    IntervalSetType is;
    is.insert(IntervalSetType::interval_type::right_open(1, 3));
    is.insert(IntervalSetType::interval_type::right_open(5, 7));
    is.insert(IntervalSetType::interval_type::right_open(9, 11));
    is.insert(IntervalSetType::interval_type::right_open(2, 6));
    is.insert(IntervalSetType::interval_type::right_open(4, 10));
    is.insert(IntervalSetType::interval_type::right_open(10, 12));

    EXPECT_EQ(is.iterative_size(), 1);
    optional_destroy(is);
}

TEST_F(IntervalSetTest, OverlapExplicit2) {
    IntervalSetType is;
    is.insert(IntervalSetType::interval_type::right_open(1, 3));
    is.insert(IntervalSetType::interval_type::right_open(5, 7));
    is.insert(IntervalSetType::interval_type::right_open(9, 11));
    is.insert(IntervalSetType::interval_type::right_open(16, 18));
    is.insert(IntervalSetType::interval_type::right_open(2, 6));
    is.insert(IntervalSetType::interval_type::right_open(11, 15));

    EXPECT_EQ(is.iterative_size(), 3);
    optional_destroy(is);
}

TEST_F(IntervalSetTest, Sequential1) {
    constexpr const int len = 100;
    IntervalSetType is;
    for (int i = 0; i < len; ++i) {
        is.insert(IntervalSetType::interval_type::right_open(i, i + 1));
        EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(i, i + 1)));
    }
    EXPECT_EQ(is.iterative_size(), 1);
    optional_destroy(is);
}

TEST_F(IntervalSetTest, Sequential2) {
    constexpr const int len = 100;
    IntervalSetType is;
    is.insert(IntervalSetType::interval_type::right_open(0, 1));
    for (int i = len; i > 0; --i) {
        is.insert(IntervalSetType::interval_type::right_open(i, i + 1));
        EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(i, i + 1)));
    }
    EXPECT_EQ(is.iterative_size(), 1);
    optional_destroy(is);
}

TEST_F(IntervalSetTest, Sequential3) {
    constexpr const int len = 100;
    IntervalSetType is;
    for (int i = 0; i < len; i += 2) {
        is.insert(IntervalSetType::interval_type::right_open(i, i + 1));
        EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(i, i + 1)));
    }
    for (int i = 1; i < len; i += 2) {
        is.insert(IntervalSetType::interval_type::right_open(i, i + 1));
        EXPECT_TRUE(IntervalSetContain(is, IntervalSetType::interval_type::right_open(i, i + 1)));
    }
    EXPECT_EQ(is.iterative_size(), 1);
    optional_destroy(is);
}

TEST_F(IntervalSetTest, Benchmark1) {
    constexpr const size_t len = 5000000u;
    size_t *starts = new size_t[len];
    size_t *ends = new size_t[len];
    for (size_t i = 0; i < len; i++) {
        starts[i] = i;
        ends[i] = i + 1;
    }
    std::shuffle(starts, starts + len, std::default_random_engine(SEED));
    std::shuffle(ends, ends + len, std::default_random_engine(SEED));
    IntervalSetType is;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < len; i++) {
        is.insert(IntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(is.iterative_size(), 1);

    start = std::clock();
    for (size_t i = 0; i < len; i++) {
        is.insert(IntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Repeat Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < len; i++) {
        bool res = IntervalSetContain(is, IntervalSetType::interval_type::right_open(starts[i], ends[i]));
        EXPECT_TRUE(res);
    }
    std::cout << "Contains: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    optional_destroy(is);
    delete[] starts;
    delete[] ends;
}

TEST_F(IntervalSetTest, Benchmark2) {
    constexpr const size_t len = 5000000u;
    size_t *starts = new size_t[len];
    size_t *ends = new size_t[len];
    std::srand(SEED);
    for (size_t i = 0; i < len; i++) {
        starts[i] = size_t(std::rand());
        ends[i] = starts[i] + (size_t(std::rand()) % 10000) + 1;
    }
    IntervalSetType is;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < len / 2; i++) {
        is.insert(IntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < len / 2; i++) {
        bool res = IntervalSetContain(is, IntervalSetType::interval_type::right_open(starts[i], ends[i]));
        EXPECT_TRUE(res);
    }
    std::cout << "Contains1: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = len / 2; i < len; i++) {
        IntervalSetContain(is, IntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Contains2: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    std::cout << "Size: " << is.iterative_size() << std::endl;

    optional_destroy(is);
    delete[] starts;
    delete[] ends;
}

/* reserved for reference */
#include <boost/icl/interval_set.hpp>
using BoostIntervalSetType = boost::icl::interval_set<size_t>;
#define BoostIntervalSetContain(set, ival) (boost::icl::contains(set, ival))
TEST_F(IntervalSetTest, Reference1) {
    constexpr const size_t len = 5000000u;
    size_t *starts = new size_t[len];
    size_t *ends = new size_t[len];
    for (size_t i = 0; i < len; i++) {
        starts[i] = i;
        ends[i] = i + 1;
    }
    std::shuffle(starts, starts + len, std::default_random_engine(SEED));
    std::shuffle(ends, ends + len, std::default_random_engine(SEED));
    BoostIntervalSetType is;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < len; i++) {
        is.insert(BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(is.iterative_size(), 1);

    start = std::clock();
    for (size_t i = 0; i < len; i++) {
        is.insert(BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Repeat Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < len; i++) {
        bool res = BoostIntervalSetContain(is, BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
        EXPECT_TRUE(res);
    }
    std::cout << "Contains: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    optional_destroy(is);
    delete[] starts;
    delete[] ends;
}

TEST_F(IntervalSetTest, Reference2) {
    constexpr const size_t len = 5000000u;
    size_t *starts = new size_t[len];
    size_t *ends = new size_t[len];
    std::srand(SEED);
    for (size_t i = 0; i < len; i++) {
        starts[i] = size_t(std::rand());
        ends[i] = starts[i] + (size_t(std::rand()) % 10000) + 1;
    }
    BoostIntervalSetType is;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < len / 2; i++) {
        is.insert(BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < len / 2; i++) {
        bool res = BoostIntervalSetContain(is, BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
        EXPECT_TRUE(res);
    }
    std::cout << "Contains1: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = len / 2; i < len; i++) {
        BoostIntervalSetContain(is, BoostIntervalSetType::interval_type::right_open(starts[i], ends[i]));
    }
    std::cout << "Contains2: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    std::cout << "Size: " << is.iterative_size() << std::endl;

    optional_destroy(is);
    delete[] starts;
    delete[] ends;
}

int main() {
    RUN_TEST(IntervalSetTest, Simple);
    RUN_TEST(IntervalSetTest, Overlap);
    RUN_TEST(IntervalSetTest, OverlapExplicit);
    RUN_TEST(IntervalSetTest, OverlapExplicit2);
    RUN_TEST(IntervalSetTest, Sequential1);
    RUN_TEST(IntervalSetTest, Sequential2);
    RUN_TEST(IntervalSetTest, Sequential3);
    RUN_TEST(IntervalSetTest, Benchmark1);
    RUN_TEST(IntervalSetTest, Benchmark2);
    RUN_TEST(IntervalSetTest, Reference1);
    RUN_TEST(IntervalSetTest, Reference2);
    return 0;
}
