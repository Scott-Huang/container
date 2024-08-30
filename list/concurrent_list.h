/**
 * Copyright © 2024 Mingwei Huang
 * Thread safe double linked list, only used for concurrent LRU with access control.
 * Multiple write/delete on one element at the same time may still not be defined.
 */

#ifndef CONTAINER_LIST_CONCURRENT_LIST_H
#define CONTAINER_LIST_CONCURRENT_LIST_H

#include <atomic>
#include <mutex>
#include <optional>

#include "../definition.h"
#include "list.h"

namespace mem_container {
template <typename T, bool use_spin_lock = false>
class ConcurrentList : public BaseObject {
public:
    class SpinLock {
    public:
        inline void lock() { while (locked.test_and_set(std::memory_order_acquire)) {} }
        inline bool try_lock() { return !locked.test_and_set(std::memory_order_acquire); }
        inline void unlock() { locked.clear(std::memory_order_release); }
    private:
        std::atomic_flag locked{ATOMIC_FLAG_INIT};
    };
    using lock_type = std::conditional_t<use_spin_lock, SpinLock, std::mutex>;
    struct ConcurrentListCell : public BaseObject {
        lock_type slock{};
        T data;
        ConcurrentListCell *prev{NULL};
        ConcurrentListCell *next{NULL};

        ConcurrentListCell(const T &data) : data(data) {}
        ConcurrentListCell(T &&data) : data(std::move(data)) {}
        template <typename... Args>
        ConcurrentListCell(Args &&...args) : data(std::forward<Args>(args)...) {}

        inline void lock() { slock.lock(); }
        inline bool try_lock() { return slock.try_lock(); }
        inline void unlock() { slock.unlock(); }
    };

    using cell_type = ConcurrentListCell;
    using iterator = ListIterator<T, ConcurrentList, cell_type>;
    using const_iterator = ListConstIterator<T, cell_type>;

    ConcurrentList() { CreateSharedMemCxt(); }
    ConcurrentList(const ConcurrentList &other) = delete;
    ConcurrentList operator=(const ConcurrentList &other) = delete;
    ConcurrentList(ConcurrentList &&other) : _head(other._head), _tail(other._tail)
    {
        other._head = other._tail = NULL;
        ExchangeMemCxt(other);
    }
    ConcurrentList operator=(ConcurrentList &&other)
    {
        if (this != &other) {
            clear();
            _head = other._head;
            _tail = other._tail;
            other._head = other._tail = NULL;
            ExchangeMemCxt(other);
        }
        return *this;
    }
    void swap(ConcurrentList &other)
    {
        std::swap(_head, other._head);
        std::swap(_tail, other._tail);
        ExchangeMemCxt(other);
    }
#ifdef NO_DESTROYER
    ~ConcurrentList() {}
#else
    ~ConcurrentList() { destroy(); }
#endif /* NO_DESTROYER */

    std::optional<T> pop_back()
    {
        std::lock_guard<lock_type> guard(_list_tail_lock);
        if (!_tail) {
            return {};
        }
        auto tmp = _tail;
        tmp.lock();
        if (!tmp->prev) {
            std::lock_guard<lock_type> guard3(_list_head_lock);
            auto res = std::make_optional(std::move(tmp->data));
            _head = _tail = NULL;
            tmp.unlock();
            optional_destroy(tmp->data);
            delete tmp;
            return res;
        }
        std::lock_guard<lock_type> guard3(tmp->prev->slock);
        tmp->prev->next = NULL;
        auto res = std::make_optional(std::move(tmp->data));
        _tail = tmp->prev;
        tmp.unlock();
        optional_destroy(tmp->data);
        delete tmp;
        return res;
    }
    std::optional<T> pop_front()
    {
        for (;;) {
            std::lock_guard<lock_type> guard(_list_head_lock);
            if (!_head) {
                return {};
            }
            auto tmp = _head;
            tmp->lock();
            if (tmp->next) {
                if (!tmp->next->try_lock()) {
                    continue;
                }
            } else if (!_list_tail_lock.try_lock()) {
                continue;
            }

            if (tmp->next) {
                tmp->next->prev = NULL;
                auto res = std::make_optional(std::move(tmp->data));
                _head = tmp->next;
                tmp->unlock();
                optional_destroy(tmp->data);
                delete tmp;
                _head->unlock();
                return res;
            }
            auto res = std::make_optional(std::move(_head->data));
            _head = _tail = NULL;
            _list_tail_lock.unlock();
            tmp->unlock();
            optional_destroy(tmp->data);
            delete tmp;
            return res;
        }
    }
    cell_type *push_back(const T &data)
    {
        cell_type *cell = alloc_new_cell(data);
        push_back(cell);
        return cell;
    }
    cell_type *push_back(T &&data)
    {
        cell_type *cell = alloc_new_cell(std::move(data));
        push_back(cell);
        return cell;
    }
    template <typename... Args>
    cell_type *emplace_back(Args &&... args)
    {
        cell_type *cell = alloc_new_cell(std::forward<Args>(args)...);
        push_back(cell);
        return cell;
    }
    cell_type *push_front(const T &data)
    {
        cell_type *cell = alloc_new_cell(data);
        push_front(cell);
        return cell;
    }
    cell_type *push_front(T &&data)
    {
        cell_type *cell = alloc_new_cell(std::move(data));
        push_front(cell);
        return cell;
    }
    template <typename... Args>
    cell_type *emplace_front(Args &&... args)
    {
        cell_type *cell = alloc_new_cell(std::forward<Args>(args)...);
        push_front(cell);
        return cell;
    }
    inline void move_back(iterator iter) { move_back(iter.cur); }

    inline void erase(iterator &iter) { erase_delete(iter.cur); }
    /* thread-unsafe, we don't expect read/write to occur under destroy() */
    void destroy()
    {
        cell_type *cur = _head;
        while (cur) {
            cell_type *tmp = cur;
            cur = cur->next;
            optional_destroy(tmp->data);
            delete tmp;
        }
        _head = _tail = NULL;
        DestroyMemCxt();
    }
    inline void clear() { destroy(); }

    inline iterator begin() { return iterator(_head); }
    inline iterator end() { return iterator(nullptr); }
    inline const_iterator cbegin() const { return const_iterator(_head); }
    inline const_iterator cend() const { return const_iterator(nullptr); }
    inline std::optional<T> front()
    {
        std::lock_guard<lock_type> guard(_list_head_lock);
        if (!_head) {
            return {};
        }
        return std::make_optional(_head->data);
    }
    inline std::optional<T> back()
    {
        std::lock_guard<lock_type> guard(_list_tail_lock);
        if (!_tail) {
            return {};
        }
        return std::make_optional(_tail->data);
    }

    inline bool empty() const { return !_head; }
    size_t thread_unsafe_size() const
    {
        size_t res = 0;
        cell_type *cur = _head;
        while (cur) {
            ++res;
            assert(res < 100000000u);
            cur = cur->next;
        }
        return res;
    }
    size_t size()
    {
        size_t res = 0;
        std::lock_guard<lock_type> guard(_list_tail_lock);
        std::lock_guard<lock_type> guard2(_list_head_lock);
        cell_type *cur = _tail;
        if (!cur) {
            return 0;
        }
        do {
            std::lock_guard<lock_type> guard3(cur->slock);
            ++res;
            assert(res < 100000000u);
            cur = cur->prev;
        } while (cur); /* bug here, don't call it anyway */
        return res;
    }
private:
    MemCxtHolder;
    cell_type *_head{NULL};
    cell_type *_tail{NULL};
    std::mutex _list_head_lock{};
    std::mutex _list_tail_lock{};

    inline cell_type *alloc_new_cell(const T &data)
    {
        cell_type *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW cell_type(data);
        } else {
            UseMemCxt();
            cell = NEW cell_type(data);
            ResetMemCxt();
        }
        return cell;
    }
    inline cell_type *alloc_new_cell(T &&data)
    {
        /* no mem alloc should happen during move */
        return NEW cell_type(data);
    }
    template <typename... Args>
    inline cell_type *alloc_new_cell(Args &&...args)
    {
        cell_type *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW cell_type(std::forward<Args>(args)...);
        } else {
            UseMemCxt();
            cell = NEW cell_type(std::forward<Args>(args)...);
            ResetMemCxt();
        }
        return cell;
    }

    void push_back(cell_type *cell)
    {
        std::lock_guard<lock_type> guard(_list_tail_lock);
        if (!_tail) {
            std::lock_guard<lock_type> guard2(_list_head_lock);
            _head = _tail = cell;
            return;
        }
        std::lock_guard<lock_type> guard2(_tail->slock);
        _tail->next = cell;
        cell->prev = _tail;
        _tail = cell;
    }
    void push_front(cell_type *cell)
    {
        for (;;) {
            std::lock_guard<lock_type> guard(_list_head_lock);
            if (!_head) {
                if (!_list_tail_lock.try_lock()) {
                    continue;
                }
                _head = _tail = cell;
                _list_tail_lock.unlock();
                return;
            }
            std::lock_guard<lock_type> guard2(_head->slock);
            cell->next = _head;
            _head->prev = cell;
            _head = cell;
            return;
        }
    }
    void erase(cell_type *cell)
    {
        for (;;) {
            std::lock_guard<lock_type> guard(cell->slock);
            if (!cell->next) {
                if (!_list_tail_lock.try_lock()) {
                    continue;
                }
                if (cell->prev) {
                    std::lock_guard<lock_type> guard3(cell->prev->slock);
                    cell->prev->next = NULL;
                    _tail = cell->prev;
                } else {
                    std::lock_guard<lock_type> guard3(_list_head_lock);
                    _head = _tail = NULL;
                }
                _list_tail_lock.unlock();
                return;
            }
            if (!cell->prev) {
                if (!_list_head_lock.try_lock()) {
                    continue;
                }
                if (!cell->next->try_lock()) {
                    _list_head_lock.unlock();
                    continue;
                }
                cell->next->prev = NULL;
                _head = cell->next;
                cell->next->unlock();
                _list_head_lock.unlock();
                return;
            }
            if (!cell->next->try_lock()) {
                continue;
            }
            std::lock_guard<lock_type> guard2(cell->prev->slock);
            cell->prev->next = cell->next;
            cell->next->prev = cell->prev;
            cell->next->unlock();
            return;
        }
    }
    inline void erase_delete(cell_type *cell)
    {
        erase(cell);
        optional_destroy(cell->data);
        delete cell;
    }
    inline void move_back(cell_type *cell)
    {
        if (cell == _tail) { /* it's likely for new accessed cell to be re-accessed */
            return;
        }
        erase(cell);
        cell->next = NULL;
        push_back(cell);
    }
};
} /* mem_container */

#endif /* CONTAINER_LIST_CONCURRENT_LIST_H */
