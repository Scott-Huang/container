/**
 * Copyright © 2024 Mingwei Huang
 * simple hash table
 */

#ifndef CONTAINER_HASHTABLE_H
#define CONTAINER_HASHTABLE_H

#include "../definition.h"
#include "../vector/vector.h"

namespace mem_container {
typedef uint32_t uint32;

template <typename T>
using HashTableVector = Vector<T, false, false>;
/* key comparison depends on equal operator */
template <typename Key, typename Value, template<typename> class VectorType = HashTableVector>
class HashTable {
public:
#if __cplusplus >= 202002L
    struct Entry { uint32 hash_value; Key key; [[no_unique_address]] Value value; bool valid; };
#else
    struct Entry { uint32 hash_value; Key key; Value value; bool valid; };
#endif /* __cplusplus c++20 or greater */
    using vector_type = VectorType<Entry>;
    /* note that iterators here are not responsible for validation */
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;

    HashTable() : HashTable(default_capacity) {}
    HashTable(size_t capacity);
    HashTable(const HashTable &) = delete;
    HashTable &operator=(const HashTable &) = delete;
    HashTable(HashTable &&other) : _table(std::move(other._table)), _size(other._size), _capacity(other._capacity)
    {
        other._size = other._capacity = 0;
    }
    HashTable &operator=(HashTable &&other)
    {
        if (this != &other) {
            swap(other);
        }
        return *this;
    }
    void swap(HashTable &other)
    {
        _table.swap(other._table);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }
    ~HashTable() {}

    bool insert(const Key &k, const Value &v);
    inline bool insert(Key &&k, const Value &v) { return insert(k, v); }
    inline bool insert(const Key &k, Value &&v) { return insert(k, v); }
    inline bool insert(Key &&k, Value &&v) { return insert(k, v); }
    void extend();

    iterator find(const Key &k);
    inline iterator find(Key &&k) { return find(k); }
    inline const_iterator cfind(const Key &k) { return const_iterator(find(k)); }
    inline const_iterator cfind(Key &&k) { return const_iterator(find(k)); }

    inline bool contains(const Key &k) { return cfind(k) != cend(); }
    inline bool contains(Key &&k) { return cfind(k) != cend(); }

    iterator begin() { return _table.begin(); }
    iterator end() { return _table.end(); }
    const_iterator cbegin() { return _table.cbegin(); }
    const_iterator cend() { return _table.cend(); }

    void destroy() { _table.destroy(); _size = _capacity = 0; }
    void clear() { _table.clear(); _size = 0; }
private:
    constexpr static const bool empty_value = std::is_empty<Value>::value;
    constexpr static const size_t default_capacity = 16;
    constexpr static const float load_factor = 0.75;
    vector_type _table{};
    size_t _size{0};
    size_t _capacity;

    static inline uint32 _hash(const Key &key)
    {
        return std::hash<Key>()(key);
    }
};

template <typename Key, template<typename> class VectorType = HashTableVector>
using HashSet = HashTable<Key, EmptyObject, VectorType>;

template <typename Key, typename Value, template<typename> class VectorType>
HashTable<Key, Value, VectorType>::HashTable(size_t capacity)
    : _capacity(capacity * 2)
{
    static_assert(std::is_standard_layout<Key>::value && std::is_standard_layout<Value>::value,
                  "only pod type allowed for disk hash table");
    _table.resize(_capacity);
}

template <typename Key, typename Value, template<typename> class VectorType>
bool HashTable<Key, Value, VectorType>::insert(const Key &k, const Value &v)
{
    bool res = false;
    Entry entry = {_hash(k), k, v, true};
    for (uint32 cur_pos = entry.hash_value;; ++cur_pos) {
        auto cur_entry = _table[cur_pos % _capacity];
        if (!cur_entry.valid) {
            _table.set(cur_pos % _capacity, entry);
            res = true;
            break;
        }
        if (entry.hash_value == cur_entry.hash_value && k == cur_entry.key) {
            res = false;
            break;
        }
    }
    if (res) {
        ++_size;
        if (_size > _capacity * load_factor) {
            extend();
        }
    }
    return res;
}

template <typename Key, typename Value, template<typename> class VectorType>
void HashTable<Key, Value, VectorType>::extend()
{
    size_t old_capacity = _capacity;
    _capacity *= 2lu;
    _table.resize(_capacity);
    for (size_t i = 0; i < old_capacity; ++i) {
        auto entry = _table[i];
        if (!entry.valid || entry.hash_value % _capacity == i) {
            continue;
        }
        _table.set(entry.hash_value % _capacity, entry);
        entry.valid = false;
        _table.set(i, entry);
    }
    
}

template <typename Key, typename Value, template<typename> class VectorType>
typename HashTable<Key, Value, VectorType>::iterator HashTable<Key, Value, VectorType>::find(const Key &k)
{
    uint32 hash_value = _hash(k);
    for (uint32 cur_pos = hash_value;; ++cur_pos) {
        auto cur_entry = _table[cur_pos % _capacity];
        if (!cur_entry.valid) {
            return end();
        }
        if (hash_value == cur_entry.hash_value && k == cur_entry.key) {
            return _table.at(cur_pos % _capacity);
        }
    }
}

} /* namespace mem_container */

#endif /* CONTAINER_HASHTABLE_H */
