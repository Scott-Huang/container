#include "../test/test.h"

#include <map>
#include <random>
#include "btree.h"

template <typename K, typename V>
static void check_invariant(const BPTree<K, V> &tree) {
    V prev;
    bool first = true;
    for (auto it = tree.cbegin(); it != tree.cend(); ++it) {
        if (!first) {
            EXPECT_TRUE(prev <= *it);
        }
        prev = *it;
        first = false;
    }
}

TEST_F(DefaultTest, Simple) {
    BPTree<int, int> bptree;
    for (int i = 0; i < 100; i++) {
        bptree.insert(i, i);
        int *v = bptree.search(i);
        EXPECT_TRUE(v != NULL);
        EXPECT_EQ(*v, i);
    }

    for (int i = 100; i < 200; i++) {
        int *v = bptree.search(i);
        EXPECT_TRUE(v == NULL);
    }

    for (int i = 0; i < 100; i++) {
        bptree.remove(i);
        int *v = bptree.search(i);
        EXPECT_TRUE(v == NULL);
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

    BPTree<int, int> bptree;
    for (size_t i = 0; i < N; i++) {
        bptree.insert(data[i], data[i]);
    }
    EXPECT_EQ(bptree.size(), N);
    check_invariant(bptree);

    for (size_t i = 0; i < N; i++) {
        int *v = bptree.search(data[i]);
        EXPECT_TRUE(v != NULL);
        EXPECT_EQ(*v, data[i]);
    }

    for (size_t i = 0; i < N / 2; i++) {
        bool res = bptree.remove(data[i]);
        EXPECT_TRUE(res);
    }
    EXPECT_EQ(bptree.size(), N / 2);
    check_invariant(bptree);

    for (size_t i = 0; i < N / 2; i++) {
        int *v = bptree.search(data[i]);
        EXPECT_TRUE(v == NULL);
    }

    for (size_t i = N / 2; i < N; i++) {
        int *v = bptree.search(data[i]);
        EXPECT_TRUE(v != NULL);
        EXPECT_EQ(*v, data[i]);
    }

    for (size_t i = N / 2; i < N; i++) {
        bool res = bptree.remove(data[i]);
        EXPECT_TRUE(res);
    }
    EXPECT_EQ(bptree.size(), 0);

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

    BPTree<int, int> bptree;
    std::clock_t start = std::clock();
    for (size_t i = 0; i < N; i++) {
        bptree.insert(data[i], data[i]);
    }
    std::cout << "Insert: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(bptree.size(), N);
    check_invariant(bptree);

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        auto *v = bptree.search(data[i]);
        EXPECT_EQ(*v, data[i]);
    }
    std::cout << "Contains: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    start = std::clock();
    for (size_t i = 0; i < N; i++) {
        bool res = bptree.remove(data[i]);
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
        m.insert(std::pair<int, int>(data[i], data[i]));
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
