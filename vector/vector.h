/**
 * Copyright © 2024 Mingwei Huang
 * Simple vector implementation
 */

#ifndef CONTAINER_VECTOR_H
#define CONTAINER_VECTOR_H

#include <new>
#include <utility>
#include <algorithm>

#include "../definition.h"

namespace mem_container {
template <typename T, bool auto_shrink = true, bool auto_init = false>
class Vector {
public:
    constexpr static const size_t default_capacity = sizeof(T) < 128 ? 16 : 4;
    explicit Vector(size_t expect_size) : _capacity(expect_size)
    {
        CreateMemCxt();
        UseMemCxt();
        _start = (T *)calloc(expect_size, sizeof(T));
        ResetMemCxt();
        _end = _start;
    }
    Vector() requires(!auto_init) :  _start(NULL), _end(NULL), _capacity(0) {}
    Vector() : Vector(default_capacity) {}
    Vector(const Vector &other) : _capacity(other.size())
    {
        CreateMemCxt();
        UseMemCxt();
        _start = (T *)calloc(_capacity, sizeof(T));
        ResetMemCxt();
        _end = _start + other.size();
        std::copy(other._start, other._end, _start);
    }
    Vector(Vector &&other) : _start(other._start), _end(other._end), _capacity(other._capacity)
    {
        other._start = NULL;
        other._end = NULL;
        other._capacity = 0;
        ExchangeMemCxt(other);
    }
    Vector &operator=(const Vector &other)
    {
        if (this != &other) {
            destroy();
            CreateMemCxt();
            _capacity = other.size();
            UseMemCxt();
            _start = (T *)calloc(_capacity, sizeof(T));
            ResetMemCxt();
            _end = _start + other.size();
            std::copy(other._start, other._end, _start);
        }
        return *this;
    }
    Vector &operator=(Vector &&other)
    {
        if (this != &other) {
            destroy();
            _start = other._start;
            _end = other._end;
            _capacity = other._capacity;
            other._start = NULL;
            other._end = NULL;
            other._capacity = 0;
            ExchangeMemCxt(other);
        }
        return *this;
    }
    ~Vector()
    {
#ifndef NO_DESTROYER
        destroy();
#endif /* NO_DESTROYER */
    }
    void swap(Vector &other)
    {
        std::swap(_start, other._start);
        std::swap(_end, other._end);
        std::swap(_capacity, other._capacity);
        ExchangeMemCxt(other);
    }

    using iterator_type = T *;
    using const_iterator_type = const T *;

    inline void reserve(size_t expect_size) { expand_to(expect_size); }
    void resize(size_t expect_size)
    {
        if (expect_size <= size()) {
            for (T *it = _start + expect_size; it < _end; ++it) {
                it->~T();
            }
            shrink_to(expect_size);
        } else {
            expand_to(expect_size);
            UseMemCxt();
            for (T *it = _end; it < _start + expect_size; ++it) {
                new (it) T();
            }
            ResetMemCxt();
        }
        _end = _start + expect_size;
    }

    inline void push_back(const T &val) { expand_to(size() + 1); *_end++ = val; }
    inline void push_back(T &&val) { expand_to(size() + 1); *_end++ = std::move(val); }
    template <typename... Args>
    inline void emplace_back(Args &&... args)
    {
        expand_to(size() + 1);
        if (std::is_trivially_constructible_v<T, Args...>) {
            new (_end++) T(std::forward<Args>(args)...);
        } else {
            UseMemCxt();
            new (_end++) T(std::forward<Args>(args)...);
            ResetMemCxt();
        }
    }
    inline void pop_back() { (--_end)->~T(); if (auto_shrink) { shrink_to(size()); } }

    inline size_t size() const { return _end - _start; }
    inline size_t capacity() const { return _capacity; }
    inline bool empty() const { return _start == _end; }

    inline T &operator[](size_t idx) { CONTAINER_ASSERT(idx < size()); return _start[idx]; }
    inline const T &operator[](size_t idx) const { CONTAINER_ASSERT(idx < size()); return _start[idx]; }
    inline T &front() { CONTAINER_ASSERT(size() > 0); return *_start; }
    inline const T &front() const { CONTAINER_ASSERT(size() > 0); return *_start; }

    inline iterator_type at(size_t idx) { return std::min(_start + idx, _end); }
    inline iterator_type begin() { return _start; }
    inline iterator_type end() { return _end; }
    inline const_iterator_type cbegin() const { return _start; }
    inline const_iterator_type cend() const { return _end; }

    /* erase not implemented since no urgent use case */

    inline void destroy()
    {
        if (_start) {
            for (T *it = _start; it != _end; ++it) {
                it->~T();
            }
            free(_start);
            _start = NULL;
            _end = NULL;
            _capacity = 0;
        }
    }
    inline void clear() { destroy(); }
private:
    T *_start;
    T *_end;
    size_t _capacity;

    void expand_to(size_t expect_size)
    {
        if (expect_size <= _capacity) {
            return;
        }
        _capacity = std::max(_capacity * 2, expect_size + 1);
        size_t old_size = size();
        UseMemCxt();
        _start = (T *)realloc(_start, _capacity * sizeof(T));
        ResetMemCxt();
        _end = _start + old_size;
    }

    void shrink_to(size_t expect_size)
    {
        if (expect_size * 2 >= _capacity) {
            return;
        }
        _capacity = std::min(_capacity / 2, expect_size + 1);
        size_t old_size = size();
        UseMemCxt();
        _start = (T *)realloc(_start, _capacity * sizeof(T));
        ResetMemCxt();
        _end = _start + old_size;
        
    }
};
} /* namespace mem_container */

#endif /* CONTAINER_VECTOR_H */
