/**
 * CopyRight © 2024 Mingwei Huang
 * Thread safe LRU cache
 */

#ifndef CONTAINER_LRU_CACHE_CONCURRENT_LRU_CACHE_H
#define CONTAINER_LRU_CACHE_CONCURRENT_LRU_CACHE_H

/* we have to use boost here, we get no other options */
#include <boost/optional.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

#include "../definition.h"
#include "../list/concurrent_list.h"

namespace mem_container {
template <typename Key, typename Value, bool move_back_on_upate = false>
class ConcurrentLRUCache {
public:
    using key_type = Key;
    using value_type = Value;
    using pair_type = struct Pair { Key key; Value value; Pair(const key_type &k, const value_type &v) : key(k), value(v) {} };
    using list_type = ConcurrentList<pair_type>;
    using list_pointer = typename list_type::iterator;
    using map_type = boost::unordered::concurrent_flat_map<key_type, list_pointer>;

    constexpr static const size_t DEFAULT_SIZE = sizeof(key_type) + sizeof(value_type) > 512 ? 200 : 1000;
    explicit ConcurrentLRUCache(size_t cache_size = DEFAULT_SIZE) : _map(cache_size), _max_size(cache_size) {}
    ConcurrentLRUCache(const ConcurrentLRUCache &other) = delete;
    ConcurrentLRUCache operator=(const ConcurrentLRUCache &other) = delete;
    ConcurrentLRUCache(ConcurrentLRUCache &&other)
    {
        _map = std::move(other._map);
        _list = std::move(other._list);
    }
    ConcurrentLRUCache operator=(ConcurrentLRUCache &&other)
    {
        if (this != &other) {
            _map = std::move(other._map);
            _list = std::move(other._list);
        }
        return std::move(*this);
    }
#ifdef NO_DESTROYER
    ~ConcurrentLRUCache() {}
#else
    ~ConcurrentLRUCache() { destroy(); };
#endif /* NO_DESTROYER */

    void put(const key_type &key, const value_type &value)
    {
        if (_map.visit(key, [this, &value](auto &x) { x.second->value = value; if (move_back_on_upate) { _list.move_back(x.second); } })) {
            return;
        }
        if (_map.try_emplace(key, _list, key, value)) {
            try_evict();
        }
    }
    inline void put(const pair_type &pair) { put(pair.key, pair.value); }
    inline void insert(const key_type &key, const value_type &value) { put(key, value); }
    inline bool get(const key_type &key, value_type &value)
    {
        return _map.cvisit(key, [this, &key, &value](const auto &x) {
            value = x.second->value;
            _list.move_back(x.second);
        });
    }
    boost::optional<value_type> get(const key_type &key)
    {
        value_type value;
        if (get(key, value)) {
            return value;
        }
        return boost::none;
    }

    inline void destroy() { optional_destroy(_map); optional_destroy(_list); }
private:
    map_type _map{};
    list_type _list{};
    size_t _max_size{};

    void try_evict()
    {
        while (_map.size() > _max_size) {
            auto k = _list.front();
            if (!k || _map.erase_if(k->key, [this](auto &x){ _list.erase(x.second); return true; }) > 0) {
                break;
            }

        }
    }
};
} /* mem_container */

#endif /* CONTAINER_LRU_CACHE_CONCURRENT_LRU_CACHE_H */
