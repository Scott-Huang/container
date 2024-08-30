#include "../test/test.h"

#include <list>
#include <random>
#include <thread>
#include <semaphore>
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

TEST_F(ConcurentTester, ConcurrentSimple) {
    constexpr size_t N = 10000;
    constexpr size_t M = 10;
    std::counting_semaphore<M> thread_sem(0);
    std::counting_semaphore<M> main_sem(0);
    std::thread threads[M];
    ConcurrentList<size_t> list;
    
    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&main_sem, &thread_sem](ConcurrentList<size_t> *list) {
            std::vector<ConcurrentList<size_t>::iterator> iters;
            for (size_t i = 0; i < N; i++) {
                iters.push_back(list->push_back(i));
            }
            for (auto &iter : iters) {
                list->move_back(iter);
            }
            thread_sem.release();
            main_sem.acquire();
            for (auto &iter : iters) {
                list->erase(iter);
            }
            thread_sem.release();
            main_sem.release();
        }, &list);
    }

    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    EXPECT_EQ(list.thread_unsafe_size(), N * M);
    main_sem.release(M);
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    EXPECT_TRUE(list.empty());
    optional_destroy(list);
    for (size_t i = 0; i < M; ++i) {
        threads[i].join();
    }
}

TEST_F(ConcurentTester, ConcurrentBenchmark) {
    constexpr size_t N = 100000;
    constexpr size_t M = 50;
    std::counting_semaphore<M> thread_sem(0);
    std::counting_semaphore<M> main_sem(0);
    std::thread threads[M];
    ConcurrentList<size_t> list;
    
    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&main_sem, &thread_sem](ConcurrentList<size_t> *list) {
            std::vector<ConcurrentList<size_t>::iterator> iters;
            iters.reserve(N);
            for (size_t i = 0; i < N; i++) {
                iters.push_back(list->push_back(i));
            }
            thread_sem.release();
            std::shuffle(iters.begin(), iters.end(), std::mt19937(std::random_device()()));
            main_sem.acquire();
            for (auto &iter : iters) {
                list->move_back(iter);
            }
            thread_sem.release();
            std::shuffle(iters.begin(), iters.end(), std::mt19937(std::random_device()()));
            main_sem.acquire();
            for (auto &iter : iters) {
                list->erase(iter);
            }
            thread_sem.release();
            main_sem.release();
        }, &list);
    }

    std::clock_t start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "push_back: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(list.thread_unsafe_size(), N * M);
    main_sem.release(M);

    start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "move_back: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(list.thread_unsafe_size(), N * M);
    main_sem.release(M);

    start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "erase: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_TRUE(list.empty());
    optional_destroy(list);
    for (size_t i = 0; i < M; ++i) {
        threads[i].join();
    }
}

TEST_F(ConcurentTester, ReferenceBenchmark) {
    constexpr size_t N = 100000;
    constexpr size_t M = 50;
    std::counting_semaphore<M> thread_sem(0);
    std::counting_semaphore<M> main_sem(0);
    std::thread threads[M];
    std::list<size_t> list;
    std::mutex list_lock;
    
    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&main_sem, &thread_sem, &list_lock](std::list<size_t> *list) {
            std::vector<std::list<size_t>::iterator> iters;
            iters.reserve(N);
            for (size_t i = 0; i < N; i++) {
                list_lock.lock();
                auto iter = list->insert(list->cend(), i);
                list_lock.unlock();
                iters.push_back(iter);
            }
            thread_sem.release();

            std::shuffle(iters.begin(), iters.end(), std::mt19937(std::random_device()()));
            main_sem.acquire();
            for (auto &iter : iters) {
                auto v = *iter;
                list_lock.lock();
                list->erase(iter);
                iter = list->insert(list->cend(), v);
                list_lock.unlock();
            }
            thread_sem.release();

            std::shuffle(iters.begin(), iters.end(), std::mt19937(std::random_device()()));
            main_sem.acquire();
            for (auto &iter : iters) {
                list_lock.lock();
                list->erase(iter);
                list_lock.unlock();
            }
            thread_sem.release();
            main_sem.release();
        }, &list);
    }

    std::clock_t start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "push_back: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(list.size(), N * M);
    main_sem.release(M);

    start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "move_back: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_EQ(list.size(), N * M);
    main_sem.release(M);

    start = std::clock();
    for (size_t i = 0; i < M; ++i) {
        thread_sem.acquire();
    }
    std::cout << "erase: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    EXPECT_TRUE(list.empty());
    optional_destroy(list);
    for (size_t i = 0; i < M; ++i) {
        threads[i].join();
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

    RUN_TEST(ConcurentTester, ConcurrentSimple);
    RUN_TEST(ConcurentTester, ConcurrentBenchmark);
    RUN_TEST(ConcurentTester, ReferenceBenchmark);
    return 0;
}
