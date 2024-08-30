/**
 * Copyright © 2024 Mingwei Huang
 * Thread safe LRU cache based on array
 */

#ifndef CONTAINER_LRU_CACHE_CONCURRENT_LRU_ARRAY_CACHE_H
#define CONTAINER_LRU_CACHE_CONCURRENT_LRU_ARRAY_CACHE_H

#include <atomic>
#include <new>
#include <thread>
#include <random>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <boost/unordered/concurrent_flat_map.hpp>

#include "../definition.h"
#include "../vector/vector.h"

namespace mem_container {
template <typename Key, typename Value, size_t nsegment = 8u>
class ConcurrentLRUArrayCache {
public:
    using lock_type = std::shared_mutex;
    using key_type = Key;
    using value_type = Value;
    using data_type = struct Data {
        Key key{};
        Value value{};
        Data(const key_type &k, const Value &v) : key(k), value(v) {};
        Data(Data &&other) : key(std::move(other.key)), value(std::move(other.value)) {}
        void destroy() { optional_destroy(key); optional_destroy(value); }
        void swap(Data &other) { std::swap(key, other.key); std::swap(value, other.value); }
    };
    using pos_type = struct Position {
        size_t pos;
        uint32_t seg;
        Position() : pos(0), seg(0) {}
        Position(size_t p, uint32_t s) : pos(p), seg(s) {}
        Position(ConcurrentLRUArrayCache &cache, const Position &other, const key_type &k, const value_type &v) : pos(other.pos), seg(other.seg)
        {
            cache._bitset[pos / SIZE_BIT] |= 1lu << (pos % SIZE_BIT);
            new (&cache._data[pos]) data_type(k, v);
        }
    };
    using map_type = boost::unordered::concurrent_flat_map<key_type, Position>;

    constexpr static const size_t SIZE_BIT = sizeof(size_t) * CHAR_BIT;
    constexpr static const size_t DEFAULT_SIZE = (sizeof(key_type) + sizeof(value_type)) > 512 ? 256 : 1024;
    explicit ConcurrentLRUArrayCache(size_t cache_size = DEFAULT_SIZE) : _capacity(cache_size), _map(cache_size * 2)
    {
        static_assert(nsegment >= 4);
        CONTAINER_ASSERT(cache_size % nsegment == 0 && cache_size > nsegment * 10);
        _data = (data_type *)malloc(sizeof(data_type) * cache_size);
        _lock = (lock_type *)malloc(sizeof(lock_type) * cache_size);
        for (size_t i = 0; i < cache_size; ++i) {
            new (_lock + i) lock_type();
        }
        uint32_t *temp_segment[nsegment];
        for (size_t i = 0; i < nsegment; ++i) {
            temp_segment[i] = (uint32_t *)malloc(sizeof(uint32_t) * cache_size);
            for (size_t j = 0; j < cache_size; ++j) {
                temp_segment[i][j] = j;
            }
            std::shuffle(temp_segment[i], temp_segment[i] + cache_size, std::default_random_engine(std::random_device{}()));
        }
        /* make each column have unique numbers */
        for (size_t i = 1; i < nsegment; ++i) {
            for (size_t j = 0; j < cache_size; ++j) {
                auto duplicated_in = [&](uint32_t val, size_t idx) {
                    for (size_t k = 0; k < i; ++k) {
                        if (temp_segment[k][idx] == val) {
                            return true;
                        }
                    }
                    return false;
                };
                if (duplicated_in(temp_segment[i][j], j)) {
                    size_t idx = (j + 1) % cache_size;
                    while (duplicated_in(temp_segment[i][idx], j) || duplicated_in(temp_segment[i][j], idx)) {
                        idx = (idx + 1) % cache_size;
                    }
                    std::swap(temp_segment[i][j], temp_segment[i][idx]);
                }
            }
        }
        _segments = (uint32_t *)malloc(sizeof(uint32_t) * cache_size * nsegment);
        for (size_t i = 0; i < cache_size; ++i) {
            for (size_t j = 0; j < nsegment; ++j) {
                _segments[i * nsegment + j] = temp_segment[j][i];
            }
            std::sort(_segments + i * nsegment, _segments + (i + 1) * nsegment);
        }
        for (size_t i = 0; i < nsegment; ++i) {
            free(temp_segment[i]);
        }
        auto bit_size = cache_size / SIZE_BIT + (cache_size % SIZE_BIT ? 1 : 0);
        _bitset = (std::atomic<size_t> *)malloc(sizeof(std::atomic<size_t>) * bit_size);
        for (size_t i = 0; i < bit_size; ++i) {
            new (_bitset + i) std::atomic<size_t>(0);
        }
    }
    ConcurrentLRUArrayCache(const ConcurrentLRUArrayCache &other) = delete;
    ConcurrentLRUArrayCache operator=(const ConcurrentLRUArrayCache &other) = delete;
    ConcurrentLRUArrayCache(ConcurrentLRUArrayCache &&other)
    {
        _capacity = other._capacity;
        _idx = other._idx.load();
        std::swap(_data, other._data);
        std::swap(_segments, other._segments);
        std::swap(_bitset, other._bitset);
        std::swap(_lock, other._lock);
        _map = std::move(other._map);
    }
    ConcurrentLRUArrayCache operator=(ConcurrentLRUArrayCache &&other)
    {
        if (this != &other) {
            _capacity = other._capacity;
            _idx = other._idx.load();
            std::swap(_data, other._data);
            std::swap(_segments, other._segments);
            std::swap(_bitset, other._bitset);
            std::swap(_lock, other._lock);
            _map = std::move(other._map);
        }
        return std::move(*this);
    }
#ifdef NO_DESTROYER
    ~ConcurrentLRUArrayCache() {}
#else
    ~ConcurrentLRUArrayCache() { destroy(); }
#endif /* NO_DESTROYER */

    void put(const key_type &key, const value_type &value)
    {
retry:
        bool res = true;
        if (_map.cvisit(key, [this, &res, &value](auto &x) {
            std::shared_lock lock(_lock[x.second.pos], std::defer_lock);
            if (!lock.try_lock() || _data[x.second.pos].key != x.first) {
                res = false;
                return;
            }
            _data[x.second.pos].value = value;
        })) {
            if (!res) {
                goto retry;
            }
            return;
        }
        auto pos = get_next_pos();
        std::unique_lock lock(_lock[pos.pos]);
        if (_bitset[pos.pos / SIZE_BIT] & (1lu << (pos.pos % SIZE_BIT))) {
            data_type data(std::move(_data[pos.pos]));
            if (_map.try_emplace(key, *this, pos, key, value)) {
                _map.erase(data.key);
                data.destroy();
            } else {
                new (_data + pos.pos) data_type(std::move(data));
            }
        } else if (_map.try_emplace(key, *this, pos, key, value)) {
            _bitset[pos.pos / SIZE_BIT] |= 1lu << (pos.pos % SIZE_BIT);
        }
    }
    inline void insert(const key_type &key, const value_type &value) { return put(key, value); }
    bool get(const key_type &key, value_type &value)
    {
        pos_type pos;
        bool res = true;
        if (!_map.cvisit(key, [this, &pos, &value, &res](const auto &x) {
            std::shared_lock lock(_lock[x.second.pos], std::defer_lock);
            if (!lock.try_lock() || _data[x.second.pos].key != x.first) {
                res = false;
                return;
            }
            value = _data[x.second.pos].value;
            pos = x.second;
        }) || !res) {
            return false;
        }
        std::unique_lock lock(_lock[pos.pos]);
        if (_data[pos.pos].key != key) {
            return true;
        }
        auto idx = std::distance(_segments + pos.seg * nsegment, std::lower_bound(_segments + pos.seg * nsegment, _segments + (pos.seg + 1) * nsegment, pos.pos));
        auto new_pos = _segments[pos.seg * nsegment + ((idx + 1) % nsegment)];
        if (pos.pos >= _idx && new_pos < pos.pos && new_pos < _idx) {
            return true;
        } else if (new_pos < pos.pos || new_pos >= _idx) {
            return true;
        }

        std::unique_lock lock2(_lock[new_pos]);
        if (!(_bitset[new_pos / SIZE_BIT] & (1lu << (new_pos % SIZE_BIT)))) {
            new (&_data[new_pos]) data_type(std::move(_data[pos.pos]));
            _map.insert_or_assign(key, pos_type(new_pos, get_random_seg()));
            _bitset[new_pos / SIZE_BIT] |= 1lu << (new_pos % SIZE_BIT);
            return true;
        }
        _map.insert_or_assign(key, pos_type(new_pos, get_random_seg()));
        _map.insert_or_assign(_data[new_pos].key, Position(pos.pos, get_random_seg()));
        _data[pos.pos].swap(_data[new_pos]);
        return true;
    }
    std::optional<value_type> get(const key_type &key)
    {
        value_type value;
        if (get(key, value)) {
            return value;
        }
        return {};
    }

    void destroy()
    {
        for (size_t i = 0; i < _capacity; ++i) {
            _data[i].destroy();
        }
        free(_data);
        free(_bitset);
        free(_lock);
        free(_segments);
        optional_destroy(_map);
    }
private:
    size_t _capacity{0};
    std::atomic<size_t> _idx{0};
    data_type *_data{NULL};
    uint32_t *_segments{NULL};
    std::atomic<size_t> *_bitset{NULL};
    std::shared_mutex *_lock{NULL};
    map_type _map;

    void evict(pos_type pos)
    {
        _lock[pos.pos].lock();
        if (!(_bitset[pos.pos / SIZE_BIT] & (1lu << (pos.pos % SIZE_BIT)))) {
            return;
        }
        _bitset[pos.pos / SIZE_BIT] &= ~(1lu << (pos.pos % SIZE_BIT));
        _map.erase(_data[pos.pos].key);
        _data[pos.pos].destroy();
    }

    inline pos_type get_next_pos()
    {
        size_t idx = _idx.fetch_add(1) % _capacity;
        return pos_type(idx, get_random_seg());
    }

    inline static uint32_t get_random_seg()
    {
        static uint32_t seg = 0;
        return ++seg % nsegment;
    }    
};
} /* mem_container */

#endif /* CONTAINER_LRU_CACHE_CONCURRENT_LRU_ARRAY_CACHE_H */
