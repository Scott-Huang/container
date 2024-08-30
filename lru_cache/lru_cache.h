/**
 * CopyRight © 2024 Mingwei Huang
 * Thread unsafe LRU cache
 */

#ifndef CONTAINER_LRU_CACHE_H
#define CONTAINER_LRU_CACHE_H

/* we have to use boost here, we get no other options */
#include <boost/optional.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include "../definition.h"
#include "../list/list.h"

namespace mem_container {
using namespace boost::unordered;
template <typename Key, typename Value>
class LRUCache {
public:
    using key_type = Key;
    using value_type = Value;
    using pair_type = struct Pair { Key key; Value value; Pair(const key_type &k, const value_type &v) : key(k), value(v) {} };
    using list_type = DLList<pair_type>;
    using list_pointer = typename list_type::iterator;
    using map_type = unordered_flat_map<key_type, list_pointer>;

    constexpr static const size_t DEFAULT_SIZE = sizeof(key_type) + sizeof(value_type) > 512 ? 200 : 1000;
    explicit LRUCache(size_t cache_size = DEFAULT_SIZE) : _max_size(cache_size), _map(cache_size) {}
    LRUCache(const LRUCache &other) = delete;
    LRUCache operator=(const LRUCache &other) = delete;
    LRUCache(LRUCache &&other)
    {
        _map = std::move(other._map);
        _list = std::move(other._list);
    }
    LRUCache operator=(LRUCache &&other)
    {
        if (this != &other) {
            _map = std::move(other._map);
            _list = std::move(other._list);
        }
        return std::move(*this);
    }
#ifdef NO_DESTROYER
    ~LRUCache() {}
#else
    ~LRUCache() { destroy(); };
#endif /* NO_DESTROYER */

    void put(const key_type &key, const value_type &value)
    {
        auto it = _map.find(key);
        if (it != _map.end()) {
            _list.move_back(it->second);
        } else {
            if (_map.size() == _max_size) {
                evict();
            }
            _map.emplace(key, _list.emplace_back(key, value));
        }
    }
    inline void put(const pair_type &pair) { put(pair.key, pair.value); }
    inline void insert(const key_type &key, const value_type &value) { put(key, value); }
    inline void insert(const pair_type &pair) { put(pair.key, pair.value); }

    bool get(const key_type &key, value_type &value)
    {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        }
        _list.move_back(it->second);
        value = it->second->value;
        return true;
    }
    boost::optional<value_type> get(const key_type &key)
    {
        value_type value;
        if (get(key, value)) {
            return value;
        }
        return boost::none;
    }

    inline size_t size() const { return _map.size(); }
    inline bool empty() const { return _map.empty(); }
    inline void destroy() { optional_destroy(_map); optional_destroy(_list); }
private:
    /* no mem cxt is required since all mem alloc is done at the container level */
    size_t _max_size;
    map_type _map{};
    list_type _list{};

    pair_type evict()
    {
        const auto &it = _list.begin();
        _map.erase(it->key);
        auto res = *it;
        _list.erase(it);
        return res;
    }
};
} /* namespace mem_container */

#endif /* CONTAINER_LRU_CACHE_H */
