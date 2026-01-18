#include "test.h"

#include <map>
#include <random>
#include <vtl/btree>

using namespace ann_helper;

template <typename K, typename V>
static void check_invariant(const Map<K, V> &tree) {
    K prev;
    bool first = true;
    for (auto it = tree.cbegin(); it != tree.cend(); ++it) {
        if (!first) {
            EXPECT_TRUE(prev <= it->first);
        }
        prev = it->first;
        first = false;
    }
}

TEST_F(DefaultTest, Simple) {
    Map<int, int> bptree;
    for (int i = 0; i < 100; i++) {
        bptree.emplace(i, i);
        auto it = bptree.find(i);
        EXPECT_TRUE(it != bptree.end());
        EXPECT_EQ(it->second, i);
    }

    for (int i = 100; i < 200; i++) {
        bool res = bptree.contains(i);
        EXPECT_FALSE(res);
    }

    for (int i = 0; i < 100; i++) {
        bptree.erase(i);
        bool res = bptree.contains(i);
        EXPECT_FALSE(res);
    }
    optional_destroy(bptree);
}

TEST_F(DefaultTest, Random) {
    constexpr size_t N = 1000000;
    int *data = new int[N];
    for (size_t i = 0; i < N; i++) {
        data[i] = int(i);
    }
    std::shuffle(data, data + N, std::default_random_engine(std::time(NULL)));

    Map<int, int> bptree;
    for (size_t i = 0; i < N; i++) {
        bptree.emplace(data[i], data[i]);
    }
    EXPECT_EQ(bptree.size(), N);
    check_invariant(bptree);

    for (size_t i = 0; i < N; i++) {
        auto it = bptree.find(data[i]);
        EXPECT_TRUE(it != bptree.end());
        EXPECT_EQ(it->second, data[i]);
    }

    for (size_t i = 0; i < N / 2; i++) {
        bool res = bptree.erase(data[i]);
        EXPECT_TRUE(res);
    }
    EXPECT_EQ(bptree.size(), N / 2);
    check_invariant(bptree);

    for (size_t i = 0; i < N / 2; i++) {
        bool res = bptree.contains(data[i]);
        EXPECT_FALSE(res);
    }

    for (size_t i = N / 2; i < N; i++) {
        auto it = bptree.find(data[i]);
        EXPECT_TRUE(it != bptree.end());
        EXPECT_EQ(it->second, data[i]);
    }

    for (size_t i = N / 2; i < N; i++) {
        bool res = bptree.erase(data[i]);
        EXPECT_TRUE(res);
    }
    EXPECT_TRUE(bptree.empty());

    optional_destroy(bptree);
    delete[] data;
}

TEST_F(DefaultTest, Benchmark1) {
    constexpr size_t N = 5000000;
    int *data = new int[N];
    for (size_t i = 0; i < N; i++) {
        data[i] = int(i);
    }
    std::shuffle(data, data + N, std::default_random_engine(std::time(NULL)));

    Map<int, int> bptree;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < N; i++) {
        bptree.emplace(data[i], data[i]);
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(bptree.size(), N);
    check_invariant(bptree);

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        auto it = bptree.find(data[i]);
        EXPECT_EQ(it->second, data[i]);
    }
    std::cout << "Contains: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        bool res = bptree.erase(data[i]);
        EXPECT_TRUE(res);
    }
    std::cout << "Remove: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(bptree.size(), 0);

    optional_destroy(bptree);
    delete[] data;
}

TEST_F(DefaultTest, Reference1) {
    constexpr size_t N = 5000000;
    int *data = new int[N];
    for (size_t i = 0; i < N; i++) {
        data[i] = int(i);
    }
    std::shuffle(data, data + N, std::default_random_engine(std::time(NULL)));
    std::map<int, int> m;

    std::clock_t start = std::clock();
    for (size_t i = 0; i < N; i++) {
        m.emplace(data[i], data[i]);
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(m.size(), N);

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        auto v = m[data[i]];
        EXPECT_EQ(v, data[i]);
    }
    std::cout << "Contains: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        m.erase(data[i]);
    }
    std::cout << "Remove: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(m.size(), 0);
}

int main() {
    RUN_TEST(DefaultTest, Simple);
    RUN_TEST(DefaultTest, Random);
    RUN_TEST(DefaultTest, Benchmark1);
    RUN_TEST(DefaultTest, Reference1);
    return 0;
}
