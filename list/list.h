/**
 * Copyright © 2024 Mingwei Huang
 * Simple list implementation
 */

#ifndef CONTAINER_LIST_H
#define CONTAINER_LIST_H

#include "../definition.h"

namespace mem_container {
template <typename T, class L, class Cell>
struct ListIterator {
    Cell *cur;
    ListIterator(Cell *cur) : cur(cur) {}
    template <typename... Args>
    ListIterator(L &list, Args &&...args) : cur(list.emplace_back(std::forward<Args>(args)...)) {}
    inline ListIterator &operator++() { cur = cur->next; return *this; }
    inline T &operator*() { return cur->data; }
    inline const T &operator*() const { return cur->data; }
    inline T *operator->() { return &cur->data; }
    inline const T *operator->() const { return &cur->data; }
    inline bool operator==(const ListIterator &other) const { return cur == other.cur; }
    inline bool operator!=(const ListIterator &other) const { return cur != other.cur; }
};

template <typename T, class Cell>
struct ListConstIterator {
    const Cell *cur;
    /* intensionally implicit */
    ListConstIterator(const Cell *cur) : cur(cur) {}
    inline ListConstIterator &operator++() { cur = cur->next; return *this; }
    inline const T &operator*() const { return cur->data; }
    inline const T *operator->() const { return &cur->data; }
    inline bool operator==(const ListConstIterator &other) const { return cur == other.cur; }
    inline bool operator!=(const ListConstIterator &other) const { return cur != other.cur; }
};

template <typename T>
class List {
public:
    struct ListCell : public BaseObject {
        T data;
        ListCell *next;

        ListCell(const T &data) : data(data), next(NULL) {}
        ListCell(T &&data) : data(std::move(data)), next(NULL) {}
        template <typename... Args>
        ListCell(Args &&...args) : data(std::forward<Args>(args)...), next(NULL) {}
    };

    using iterator = ListIterator<T, List, ListCell>;
    using const_iterator = ListConstIterator<T, ListCell>;

    List() { CreateMemCxt(); }
    List(const List &other) : List()
    {
        ListCell *cur = other._head;
        while (cur) {
            push_back(cur->data);
            cur = cur->next;
        }
    }
    List(List &&other) : _head(other._head), _tail(other._tail)
    {
        other._head = other._tail = NULL;
        ExchangeMemCxt(other);
    }
    List &operator=(const List &other)
    {
        if (this != &other) {
            destroy();
            CreateMemCxt();
            ListCell *cur = other._head;
            while (cur) {
                push_back(cur->data);
                cur = cur->next;
            }
        }
        return *this;
    }
    List &operator=(List &&other)
    {
        if (this != &other) {
            destroy();
            swap(other);
        }
        return *this;
    }
    void swap(List &other)
    {
        std::swap(_head, other._head);
        std::swap(_tail, other._tail);
        ExchangeMemCxt(other);
    }

    ListCell *push_back(const T &data)
    {
        ListCell *cell = alloc_new_cell(data);
        if (!_head) {
            _head = _tail = cell;
        } else {
            _tail->next = cell;
            _tail = cell;
        }
        return cell;
    }
    ListCell *push_front(const T &data)
    {
        ListCell *cell = alloc_new_cell(data);
        if (!_head) {
            _head = _tail = cell;
        } else {
            cell->next = _head;
            _head = cell;
        }
        return cell;
    }
    template <typename... Args>
    ListCell *emplace_back(Args &&...args)
    {
        ListCell *cell = alloc_new_cell(std::forward<Args>(args)...);
        if (!_head) {
            _head = _tail = cell;
        } else {
            _tail->next = cell;
            _tail = cell;
        }
        return cell;
    }
    template <typename... Args>
    ListCell *emplace_front(Args &&...args)
    {
        ListCell *cell = alloc_new_cell(std::forward<Args>(args)...);
        if (!_head) {
            _head = _tail = cell;
        } else {
            cell->next = _head;
            _head = cell;
        }
        return cell;
    }
    void pop_back()
    {
        if (!_head) {
            return;
        }
        if (_head == _tail) {
            optional_destroy(_head);
            delete _head;
            _head = _tail = NULL;
            return;
        }
        ListCell *cur = _head;
        while (cur->next != _tail) {
            cur = cur->next;
        }
        optional_destroy(_tail);
        delete _tail;
        _tail = cur;
        _tail->next = NULL;
    }
    void pop_front()
    {
        if (!_head) {
            return;
        }
        if (_head == _tail) {
            optional_destroy(_head);
            delete _head;
            _head = _tail = NULL;
            return;
        }
        ListCell *tmp = _head;
        _head = _head->next;
        optional_destroy(tmp);
        delete tmp;
    }

    inline size_t size() const
    {
        size_t cnt = 0;
        ListCell *cur = _head;
        while (cur) {
            ++cnt;
            cur = cur->next;
        }
        return cnt;
    }
    inline bool empty() const { return _head == NULL; }

    inline iterator begin() { return _head; }
    inline iterator end() { return NULL; }
    inline const_iterator cbegin() const { return _head; }
    inline const_iterator cend() const { return NULL; }
    inline T &front() { CONTAINER_ASSERT(_head); return _head->data; }
    inline const T &front() const { CONTAINER_ASSERT(_head); return _head->data; }
    inline T &back() { CONTAINER_ASSERT(_tail); return _tail->data; }
    inline const T &back() const { CONTAINER_ASSERT(_tail); return _tail->data; }

    inline void erase(iterator it) { erase(it.cur); }
    void destroy()
    {
        ListCell *cur = _head;
        while (cur) {
            ListCell *tmp = cur;
            cur = cur->next;
            optional_destroy(tmp);
            delete tmp;
        }
        _head = _tail = NULL;
        DestroyMemCxt();
    }
    inline void clear() { destroy(); }
private:
    MemCxtHolder;
    ListCell *_head{NULL};
    ListCell *_tail{NULL};

    inline ListCell *alloc_new_cell(const T &data)
    {
        ListCell *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW ListCell(data);
        } else {
            UseMemCxt();
            cell = NEW ListCell(data);
            ResetMemCxt();
        }
        return cell;
    }
    inline ListCell *alloc_new_cell(T &&data)
    {
        /* no mem alloc should happen during move */
        return NEW ListCell(data);
    }
    template <typename... Args>
    inline ListCell *alloc_new_cell(Args &&...args)
    {
        ListCell *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW ListCell(std::forward<Args>(args)...);
        } else {
            UseMemCxt();
            cell = NEW ListCell(std::forward<Args>(args)...);
            ResetMemCxt();
        }
        return cell;
    }

    void erase(ListCell *cell)
    {
        if (cell == _head) {
            _head = _head->next;
            optional_destroy(cell);
            delete cell;
            return;
        }
        ListCell *cur = _head;
        while (cur->next != cell) {
            cur = cur->next;
        }
        cur->next = cell->next;
        optional_destroy(cell);
        delete cell;
    }
};

template <typename T>
class DLList {
public:
    struct DLListCell : public BaseObject {
        T data;
        DLListCell *prev;
        DLListCell *next;

        DLListCell(const T &data) : data(data), prev(NULL), next(NULL) {}
        DLListCell(T &&data) : data(std::move(data)), prev(NULL), next(NULL) {}
        template <typename... Args>
        DLListCell(Args &&...args) : data(std::forward<Args>(args)...), prev(NULL), next(NULL) {}
    };

    using iterator = ListIterator<T, DLList, DLListCell>;
    using const_iterator = ListConstIterator<T, DLListCell>;

    DLList() { CreateMemCxt(); }
    DLList(const DLList &other) : DLList()
    {
        DLListCell *cur = other._head;
        while (cur) {
            push_back(cur->data);
            cur = cur->next;
        }
    }
    DLList(DLList &&other) : _head(other._head), _tail(other._tail)
    {
        other._head = other._tail = NULL;
        ExchangeMemCxt(other);
    }
    DLList &operator=(const DLList &other)
    {
        if (this != &other) {
            destroy();
            CreateMemCxt();
            DLListCell *cur = other._head;
            while (cur) {
                push_back(cur->data);
                cur = cur->next;
            }
        }
        return *this;
    }
    DLList &operator=(DLList &&other)
    {
        if (this != &other) {
            destroy();
            swap(other);
        }
        return *this;
    }
    void swap(DLList &other)
    {
        std::swap(_head, other._head);
        std::swap(_tail, other._tail);
        ExchangeMemCxt(other);
    }

    DLListCell *push_back(const T &data)
    {
        DLListCell *cell = alloc_new_cell(data);
        if (!_head) {
            _head = _tail = cell;
        } else {
            _tail->next = cell;
            cell->prev = _tail;
            _tail = cell;
        }
        return cell;
    }
    DLListCell *push_front(const T &data)
    {
        DLListCell *cell = alloc_new_cell(data);
        if (!_head) {
            _head = _tail = cell;
        } else {
            cell->next = _head;
            _head->prev = cell;
            _head = cell;
        }
        return cell;
    }
    template <typename... Args>
    DLListCell *emplace_back(Args &&...args)
    {
        DLListCell *cell = alloc_new_cell(std::forward<Args>(args)...);
        if (!_head) {
            _head = _tail = cell;
        } else {
            _tail->next = cell;
            cell->prev = _tail;
            _tail = cell;
        }
        return cell;
    }
    template <typename... Args>
    DLListCell *emplace_front(Args &&...args)
    {
        DLListCell *cell = alloc_new_cell(std::forward<Args>(args)...);
        if (!_head) {
            _head = _tail = cell;
        } else {
            cell->next = _head;
            _head->prev = cell;
            _head = cell;
        }
        return cell;
    }
    void pop_back()
    {
        if (!_head) {
            return;
        }
        if (_head == _tail) {
            optional_destroy(_head);
            delete _head;
            _head = _tail = NULL;
            return;
        }
        DLListCell *tmp = _tail;
        _tail = _tail->prev;
        optional_destroy(tmp);
        delete tmp;
        _tail->next = NULL;
    }
    void pop_front()
    {
        if (!_head) {
            return;
        }
        if (_head == _tail) {
            optional_destroy(_head);
            delete _head;
            _head = _tail = NULL;
            return;
        }
        DLListCell *tmp = _head;
        _head = _head->next;
        optional_destroy(tmp);
        delete tmp;
        _head->prev = NULL;
    }

    inline size_t size() const
    {
        size_t cnt = 0;
        DLListCell *cur = _head;
        while (cur) {
            ++cnt;
            cur = cur->next;
        }
        return cnt;
    }
    inline bool empty() const { return _head == NULL; }

    inline iterator begin() { return _head; }
    inline iterator end() { return NULL; }
    inline const_iterator cbegin() const { return _head; }
    inline const_iterator cend() const { return NULL; }
    inline T &front() { CONTAINER_ASSERT(_head); return _head->data; }
    inline const T &front() const { CONTAINER_ASSERT(_head); return _head->data; }
    inline T &back() { CONTAINER_ASSERT(_tail); return _tail->data; }
    inline const T &back() const { CONTAINER_ASSERT(_tail); return _tail->data; }

    inline void move_back(iterator it) { move_back(it.cur); }
    inline void move_back(const_iterator it) { move_back(iterator(it.cur)); }
    inline void erase(iterator it) { erase(it.cur); }
    void destroy()
    {
        DLListCell *cur = _head;
        while (cur) {
            DLListCell *tmp = cur;
            cur = cur->next;
            optional_destroy(tmp);
            delete tmp;
        }
        _head = _tail = NULL;
        DestroyMemCxt();
    }
    inline void clear() { destroy(); }
private:
    MemCxtHolder;
    DLListCell *_head{NULL};
    DLListCell *_tail{NULL};

    inline DLListCell *alloc_new_cell(const T &data)
    {
        DLListCell *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW DLListCell(data);
        } else {
            UseMemCxt();
            cell = NEW DLListCell(data);
            ResetMemCxt();
        }
        return cell;
    }
    inline DLListCell *alloc_new_cell(T &&data)
    {
        /* no mem alloc should happen during move */
        return NEW DLListCell(data);
    }
    template <typename... Args>
    inline DLListCell *alloc_new_cell(Args &&...args)
    {
        DLListCell *cell;
        if (std::is_standard_layout<T>::value) {
            cell = NEW DLListCell(std::forward<Args>(args)...);
        } else {
            UseMemCxt();
            cell = NEW DLListCell(std::forward<Args>(args)...);
            ResetMemCxt();
        }
        return cell;
    }

    void erase(DLListCell *cell)
    {
        if (cell == _head) {
            _head = _head->next;
            optional_destroy(cell);
            delete cell;
            return;
        }
        if (cell == _tail) {
            _tail = _tail->prev;
            optional_destroy(cell);
            delete cell;
            return;
        }
        cell->prev->next = cell->next;
        cell->next->prev = cell->prev;
        optional_destroy(cell);
        delete cell;
    }
    void move_back(DLListCell *cell)
    {
        if (cell == _tail) {
            return;
        }
        if (cell == _head) {
            _head = _head->next;
            _head->prev = NULL;
            _tail->next = cell;
            cell->prev = _tail;
            cell->next = NULL;
            _tail = cell;
            return;
        }
        cell->prev->next = cell->next;
        cell->next->prev = cell->prev;
        _tail->next = cell;
        cell->prev = _tail;
        cell->next = NULL;
        _tail = cell;
    }
};
} /* mem_container */

#endif /* CONTAINER_LIST_H */
