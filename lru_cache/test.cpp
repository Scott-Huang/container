#include "../test/test.h"

#include <atomic>
#include <thread>
#include <semaphore>
#include <boost/compute/detail/lru_cache.hpp>

#include "lru_cache.h"
#include "concurrent_lru_cache.h"
#include "concurrent_lru_array_cache.h"

using namespace mem_container;
using namespace boost::compute::detail;

template class ConcurrentLRUArrayCache<size_t, size_t>;

TEST_F(DefaultTester, RepeatedInsert) {
    LRUCache<int, int> cache(3);
    cache.insert(1, 1);
    cache.insert(1, 1);
    cache.insert(1, 1);
    EXPECT_EQ(cache.size(), 1);
}

TEST_F(DefaultTester, Insert) {
    LRUCache<int, int> cache(3);
    cache.insert(1, 1);
    cache.insert(2, 2);
    cache.insert(3, 3);
    EXPECT_EQ(cache.size(), 3);
}

TEST_F(DefaultTester, Get) {
    LRUCache<int, int> cache(3);
    cache.insert(1, 1);
    cache.insert(2, 2);
    cache.insert(3, 3);
    EXPECT_EQ(cache.get(1), 1);
    EXPECT_EQ(cache.get(2), 2);
    EXPECT_EQ(cache.get(3), 3);
}

TEST_F(DefaultTester, InsertEvict) {
    LRUCache<int, int> cache(3);
    cache.insert(1, 1);
    cache.insert(2, 2);
    cache.insert(3, 3);
    cache.insert(4, 4);
    EXPECT_EQ(cache.size(), 3);
    EXPECT_FALSE(cache.get(1));
}

TEST_F(DefaultTester, Benchmark) {
    constexpr size_t cache_size = 10000;
    constexpr size_t N = 10'000'000;
    constexpr size_t pN = 5'000;
    size_t *data = new size_t[pN];
    for (size_t i = 0; i < pN; ++i) {
        data[i] = std::rand() % N;
    }
    LRUCache<size_t, size_t> cache(cache_size);
    for (size_t i = 0; i < pN; ++i) {
        cache.insert(data[i], data[i]);
    }
    for (size_t i = 0; i < N; ++i) {
        cache.insert(i, i);
        cache.get(data[i % pN]);
    }
}

TEST_F(DefaultTester, Reference) {
    constexpr size_t cache_size = 10000;
    constexpr size_t N = 10'000'000;
    constexpr size_t pN = 5'000;
    size_t *data = new size_t[pN];
    for (size_t i = 0; i < pN; ++i) {
        data[i] = std::rand() % N;
    }
    lru_cache<size_t, size_t> cache(cache_size);
    for (size_t i = 0; i < pN; ++i) {
        cache.insert(data[i], data[i]);
    }
    for (size_t i = 0; i < N; ++i) {
        cache.insert(i, i);
        cache.get(data[i % pN]);
    }
}

TEST_F(ConcurrentTester, ConcurrentSimple) {
    constexpr size_t N = 100'000;
    constexpr size_t Np = 100;
    constexpr size_t M = 10;
    std::thread threads[M];
    ConcurrentLRUCache<size_t, size_t> cache(Np * 2 * M);
    std::atomic<size_t> hit_count(0);

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            size_t *data = new size_t[N];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }

            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(pdata[i % Np]);
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    hit_count_local++;
                } else {
                    cache.put(pdata[i % Np], pdata[i % Np]);
                }
                cache.put(data[i], data[i]);
            }

            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M; ++i) {
        threads[i].join();
    }
    std::cout << "hit_reate: " << double(hit_count) / (N * M) << std::endl;
    EXPECT_TRUE(hit_count > N * M / 2);
}

TEST_F(ConcurrentTester, ConcurrentArraySimple) {
    constexpr size_t N = 100'000;
    constexpr size_t Np = 1000;
    constexpr size_t M = 10;
    std::thread threads[M];
    ConcurrentLRUArrayCache<size_t, size_t> cache(Np * 5 * M);
    std::atomic<size_t> hit_count(0);

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            size_t *data = new size_t[N];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }

            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(pdata[i % Np]);
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    hit_count_local++;
                } else {
                    cache.put(pdata[i % Np], pdata[i % Np]);
                }
                cache.put(data[i], data[i]);
            }

            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M; ++i) {
        threads[i].join();
    }
    std::cout << "hit_reate: " << double(hit_count) / (N * M) << std::endl;
    EXPECT_TRUE(hit_count > N * M / 3);
}

TEST_F(ConcurrentTester, ConcurrentBenchmark) {
    constexpr size_t N = 1'000'000;
    constexpr size_t Np = 2000;
    constexpr size_t M = 10;
    constexpr size_t Mp = 20;
    constexpr size_t Mpp = 0;
    auto start_time = std::chrono::system_clock::now();
    std::thread threads[M + Mp + Mpp];
    ConcurrentLRUCache<size_t, size_t> cache(Np * 2 * Mp + 1);
    std::atomic<size_t> hit_count(0);
    std::cout << "init time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count() << std::endl;

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache]() {
            size_t *data = new size_t[N];
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(data[i]);
                if (!res) {
                    cache.insert(data[i], data[i]);
                }
            }
        });
    }

    for (size_t i = 0; i < Mp; ++i) {
        threads[i + M] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(pdata[i % Np]);
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    ++hit_count_local;
                } else {
                    cache.insert(pdata[i % Np], pdata[i % Np]);
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < Mpp; ++i) {
        threads[i + M + Mp] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(SIZE_MAX);
                if (res) {
                    EXPECT_EQ(*res, SIZE_MAX);
                    ++hit_count_local;
                } else {
                    cache.insert(SIZE_MAX, SIZE_MAX);
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M + Mp + Mpp; ++i) {
        threads[i].join();
    }
    double hit_rate = Mp + Mpp > 0 ? double(hit_count) / (N * (Mp + Mpp)) : 1;
    std::cout << "hit_reate: " << hit_rate << std::endl;
    EXPECT_TRUE(hit_rate > 0.5);
}

TEST_F(ConcurrentTester, ConcurrentArrayBenchmark) {
    constexpr size_t N = 1'000'000;
    constexpr size_t Np = 2000;
    constexpr size_t M = 10;
    constexpr size_t Mp = 20;
    constexpr size_t Mpp = 2;
    auto start_time = std::chrono::system_clock::now();
    std::thread threads[M + Mp + Mpp];
    ConcurrentLRUArrayCache<size_t, size_t> cache(std::max(800lu, Np * 3lu * Mp + 2lu));
    std::atomic<size_t> hit_count(0);
    std::cout << "init time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count() << "ms" << std::endl;

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache]() {
            size_t *data = new size_t[N];
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(data[i]);
                if (!res) {
                    cache.insert(data[i], data[i]);
                }
            }
        });
    }

    for (size_t i = 0; i < Mp; ++i) {
        threads[i + M] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(pdata[i % Np]);
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    ++hit_count_local;
                } else {
                    cache.insert(pdata[i % Np], pdata[i % Np]);
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < Mpp; ++i) {
        threads[i + M + Mp] = std::thread([&cache, &hit_count]() {
            size_t hit_count_local = 0;
            for (size_t i = 0; i < N; ++i) {
                auto res = cache.get(SIZE_MAX);
                if (res) {
                    EXPECT_EQ(*res, SIZE_MAX);
                    ++hit_count_local;
                } else {
                    cache.insert(SIZE_MAX, SIZE_MAX);
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M + Mp + Mpp; ++i) {
        threads[i].join();
    }
    double hit_rate = Mp + Mpp > 0 ? double(hit_count) / (N * (Mp + Mpp)) : 1;
    std::cout << "hit_reate: " << hit_rate << std::endl;
    EXPECT_TRUE(hit_rate > 0.5);
}

TEST_F(ConcurrentTester, ConcurrentReference) {
    constexpr size_t N = 1'000'000;
    constexpr size_t Np = 2000;
    constexpr size_t M = 10;
    constexpr size_t Mp = 20;
    constexpr size_t Mpp = 2;
    auto start_time = std::chrono::system_clock::now();
    std::thread threads[M + Mp + Mpp];
    lru_cache<size_t, size_t> cache(Np * 2 * Mp + 1);
    std::atomic<size_t> hit_count(0);
    std::mutex lock;
    std::cout << "init time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count() << std::endl;

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache, &lock]() {
            size_t *data = new size_t[N];
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(data[i]);
                lock.unlock();
                if (!res) {
                    lock.lock();
                    cache.insert(data[i], data[i]);
                    lock.unlock();
                }
            }
        });
    }

    for (size_t i = 0; i < Mp; ++i) {
        threads[i + M] = std::thread([&cache, &lock, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(pdata[i % Np]);
                lock.unlock();
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    ++hit_count_local;
                } else {
                    lock.lock();
                    cache.insert(pdata[i % Np], pdata[i % Np]);
                    lock.unlock();
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < Mpp; ++i) {
        threads[i + M + Mp] = std::thread([&cache, &hit_count, &lock]() {
            size_t hit_count_local = 0;
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(SIZE_MAX);
                lock.unlock();
                if (res) {
                    EXPECT_EQ(*res, SIZE_MAX);
                    ++hit_count_local;
                } else {
                    lock.lock();
                    cache.insert(SIZE_MAX, SIZE_MAX);
                    lock.unlock();
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M + Mp + Mpp; ++i) {
        threads[i].join();
    }
    double hit_rate = Mp + Mpp > 0 ? double(hit_count) / (N * (Mp + Mpp)) : 1;
    std::cout << "hit_reate: " << hit_rate << std::endl;
    EXPECT_TRUE(hit_rate > 0.5);
}

TEST_F(ConcurrentTester, ConcurrentReference2) {
    constexpr size_t N = 1'000'000;
    constexpr size_t Np = 2000;
    constexpr size_t M = 10;
    constexpr size_t Mp = 20;
    constexpr size_t Mpp = 2;
    auto start_time = std::chrono::system_clock::now();
    std::thread threads[M + Mp + Mpp];
    LRUCache<size_t, size_t> cache(Np * 2 * Mp + 1);
    std::atomic<size_t> hit_count(0);
    std::mutex lock;
    std::cout << "init time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count() << std::endl;

    for (size_t i = 0; i < M; ++i) {
        threads[i] = std::thread([&cache, &lock]() {
            size_t *data = new size_t[N];
            for (size_t i = 0; i < N; ++i) {
                data[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(data[i]);
                lock.unlock();
                if (!res) {
                    lock.lock();
                    cache.insert(data[i], data[i]);
                    lock.unlock();
                }
            }
        });
    }

    for (size_t i = 0; i < Mp; ++i) {
        threads[i + M] = std::thread([&cache, &lock, &hit_count]() {
            size_t hit_count_local = 0;
            size_t *pdata = new size_t[Np];
            for (size_t i = 0; i < Np; ++i) {
                pdata[i] = std::rand();
            }
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(pdata[i % Np]);
                lock.unlock();
                if (res) {
                    EXPECT_EQ(*res, pdata[i % Np]);
                    ++hit_count_local;
                } else {
                    lock.lock();
                    cache.insert(pdata[i % Np], pdata[i % Np]);
                    lock.unlock();
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < Mpp; ++i) {
        threads[i + M + Mp] = std::thread([&cache, &lock, &hit_count]() {
            size_t hit_count_local = 0;
            for (size_t i = 0; i < N; ++i) {
                lock.lock();
                auto res = cache.get(SIZE_MAX);
                lock.unlock();
                if (res) {
                    EXPECT_EQ(*res, SIZE_MAX);
                    ++hit_count_local;
                } else {
                    lock.lock();
                    cache.insert(SIZE_MAX, SIZE_MAX);
                    lock.unlock();
                }
            }
            hit_count += hit_count_local;
        });
    }

    for (size_t i = 0; i < M + Mp + Mpp; ++i) {
        threads[i].join();
    }
    double hit_rate = Mp + Mpp > 0 ? double(hit_count) / (N * (Mp + Mpp)) : 1;
    std::cout << "hit_reate: " << hit_rate << std::endl;
    EXPECT_TRUE(hit_rate > 0.5);
}

int main() {
    RUN_TEST(DefaultTester, RepeatedInsert);
    RUN_TEST(DefaultTester, Insert);
    RUN_TEST(DefaultTester, Get);
    RUN_TEST(DefaultTester, InsertEvict);
    RUN_TEST(DefaultTester, Benchmark);
    RUN_TEST(DefaultTester, Reference);
    RUN_TEST(ConcurrentTester, ConcurrentSimple);
    RUN_TEST(ConcurrentTester, ConcurrentArraySimple);
    RUN_TEST(ConcurrentTester, ConcurrentBenchmark);
    RUN_TEST(ConcurrentTester, ConcurrentArrayBenchmark);
    RUN_TEST(ConcurrentTester, ConcurrentReference);
    RUN_TEST(ConcurrentTester, ConcurrentReference2);

    
    return 0;
}
