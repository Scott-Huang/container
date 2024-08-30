/**
 * Copyright © 2024 Mingwei Huang
 * B+ Tree implementation, thread unsafe, and comparator is not supported
 */

#ifndef CONTAINER_BTREE_H
#define CONTAINER_BTREE_H

#include <type_traits>

#include "../definition.h"

namespace mem_container {
template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE = 16>
class BPTree {
public:
    struct Node : public BaseObject {
        bool is_leaf;
        size_t size{0};
        K key[MAX_BPTREE_NODE_SIZE];
        Node(bool leaf) : is_leaf(leaf) {}
        virtual void destroy() {}
        virtual ~Node() {}

        /* sequential scan for small quantities */
        inline size_t item_index_of(const K &x) const
        {
            size_t i = 0;
            while (i < size && key[i] < x) {
                ++i;
            }
            return i;
        }
        inline size_t child_index_of(const K &x) const
        {
            size_t i;
            for (i = 0; i < size; ++i) {
                if (x < key[i]) {
                    break;
                }
            }
            return i;
        }
    };
    struct InternalNode : public Node {
        Node *ptr[MAX_BPTREE_NODE_SIZE + 1];
        InternalNode() : Node(false) {}
        void destroy() override { Node::destroy(); }
    };
    struct LeafNode : public Node {
        V values[MAX_BPTREE_NODE_SIZE];
        LeafNode *next{NULL};
        LeafNode *prev{NULL};
        LeafNode() : Node(true) {}
        void destroy() override { Node::destroy(); }
    };
    using node_type = Node;
    using internal_node_type = InternalNode;
    using leaf_node_type = LeafNode;

    struct iterator {
        leaf_node_type *node;
        size_t index;
        iterator(leaf_node_type *n, size_t i) : node(n), index(i) {}
        iterator &operator++()
        {
            if (index + 1 < node->size) {
                ++index;
            } else {
                node = node->next;
                index = 0;
            }
            return *this;
        }
        iterator &operator--()
        {
            if (index > 0) {
                --index;
            } else {
                node = node->prev;
                index = node->size - 1;
            }
            return *this;
        }
        bool operator==(const iterator &other) const { return node == other.node && index == other.index; }
        bool operator!=(const iterator &other) const { return !(*this == other); }
        V &operator*() { return node->values[index]; }
        V &value() { return node->values[index]; }
        const K &key() const { return node->key[index]; }
    };
    struct const_iterator {
        const leaf_node_type *node;
        size_t index;
        const_iterator(const leaf_node_type *n, size_t i) : node(n), index(i) {}
        const_iterator &operator++()
        {
            if (index + 1 < node->size) {
                ++index;
            } else {
                node = node->next;
                index = 0;
            }
            return *this;
        }
        const_iterator &operator--()
        {
            if (index > 0) {
                --index;
            } else {
                node = node->prev;
                index = node->size - 1;
            }
            return *this;
        }
        bool operator==(const const_iterator &other) const { return node == other.node && index == other.index; }
        bool operator!=(const const_iterator &other) const { return !(*this == other); }
        const V &operator*() const { return node->values[index]; }
        const V &value() const { return node->values[index]; }
        const K &key() const { return node->key[index]; }
    };

    iterator begin() { return iterator(find_start_leaf(), 0); }
    iterator end() const { return iterator(NULL, 0); }
    const_iterator cbegin() const { return const_iterator(find_start_leaf(), 0); }
    const_iterator cend() const { return const_iterator(NULL, 0); }

    BPTree()
    {
        static_assert(std::is_standard_layout<K>::value && std::is_standard_layout<V>::value, "bptree only support pod types");
        CreateMemCxt();
    }
    BPTree(const BPTree &) = delete;
    BPTree &operator=(const BPTree &) = delete;
    BPTree(BPTree &&other) : _root(other._root)
    {
        other._root = NULL;
        ExchangeMemCxt(other);
    }
    BPTree &operator=(BPTree &&other)
    {
        if (this != &other) {
            destroy();
            _root = other._root;
            other._root = NULL;
            ExchangeMemCxt(other);
        }
        return *this;
    }
    ~BPTree()
    {
#ifndef NO_DESTROYER
        destroy();
#endif /* NO_DESTROYER */
    }
    iterator find_left(const K &x);
    const_iterator cfind_left(const K &x) const;
    iterator find(const K &x);
    const_iterator cfind(const K &x) const;
    V *search(const K &x);
    const V *search(const K &) const;
    V &operator[](const K &x)
    {
        V *res = search(x);
        if (res) {
            return *res;
        }
        insert(x, V());
        return *search(x);
    }
    /* segfault if key does not exist */
    inline const V &operator[](const K &x) const { return *search(x); }
    void insert(const K &, const V &);
    void remove(iterator it);
    bool remove(const K &);
    size_t size() const;
    inline bool empty() const { return !_root; }
#ifdef BTREE_DEBUG
    __attribute__((noinline, used))
    void display() const { display_internal(_root); }
    void display_internal(const node_type *node) const;
#endif /* BTREE_DEBUG */
    inline void check_invariant() const { check_invariant(_root); }
    inline void destroy()
    {
        clean_up(_root);
        _root = NULL;
        DestroyMemCxt();
    }
private:
    MemCxtHolder;
    node_type *_root{NULL};
    void insert_internal(const K &, const V &, internal_node_type *, node_type *);
    node_type *insert_split(const K &, const V &, node_type *, node_type *, K &split_key);
    void remove_internal(const K &, internal_node_type *, node_type *);
    internal_node_type *find_parent(internal_node_type *, const node_type *) const;
    leaf_node_type *find_start_leaf() const;
    void clean_up(node_type *);
#ifdef BTREE_VERIFY_DATA
    K check_invariant(const node_type *) const;
#else
    void check_invariant(const node_type *) const {}
#endif /* BTREE_VERIFY_DATA */
};
} /* namespace mem_container */

/* place for implementation */

#include <cstring>

#ifdef BTREE_DEBUG
#include <iostream>
#endif /* BTREE_DEBUG */

using namespace mem_container;

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::leaf_node_type *BPTree<K, V, MAX_BPTREE_NODE_SIZE>::find_start_leaf() const
{
    if (!_root) {
        return NULL;
    }
    node_type *cursor = _root;
    while (!cursor->is_leaf) {
        cursor = reinterpret_cast<internal_node_type *>(cursor)->ptr[0];
    }
    return reinterpret_cast<leaf_node_type *>(cursor);
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
size_t BPTree<K, V, MAX_BPTREE_NODE_SIZE>::size() const
{
    size_t res = 0;
    auto cursor = cbegin().node;
    while (cursor) {
        res += cursor->size;
        cursor = cursor->next;
    }
    return res;
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
V *BPTree<K, V, MAX_BPTREE_NODE_SIZE>::search(const K &x)
{
    auto it = find(x);
    if (it == end()) {
        return NULL;
    }
    return &(*it);
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
const V *BPTree<K, V, MAX_BPTREE_NODE_SIZE>::search(const K &x) const
{
    auto it = cfind(x);
    if (it == cend()) {
        return NULL;
    }
    return &(*it);
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::iterator BPTree<K, V, MAX_BPTREE_NODE_SIZE>::find_left(const K &x)
{
    if (!_root) {
        return end();
    }
    node_type *cursor = _root;
    while (!cursor->is_leaf) {
        cursor = reinterpret_cast<internal_node_type *>(cursor)->ptr[cursor->child_index_of(x)];
    }
    size_t i = cursor->child_index_of(x);
    if (i == 0) {
        leaf_node_type *node = reinterpret_cast<leaf_node_type *>(cursor)->prev;
        return node ? iterator(node, node->size - 1) : end();
    }
    return iterator(reinterpret_cast<leaf_node_type *>(cursor), i - 1);
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::const_iterator BPTree<K, V, MAX_BPTREE_NODE_SIZE>::cfind_left(const K &x) const
{
    if (!_root) {
        return cend();
    }
    node_type *cursor = _root;
    while (!cursor->is_leaf) {
        cursor = reinterpret_cast<internal_node_type *>(cursor)->ptr[cursor->child_index_of(x)];
    }
    size_t i = cursor->child_index_of(x);
    if (i == 0) {
        leaf_node_type *node = reinterpret_cast<leaf_node_type *>(cursor)->prev;
        return node ? const_iterator(node, node->size - 1) : cend();
    }
    return const_iterator(reinterpret_cast<leaf_node_type *>(cursor), i - 1);
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::iterator BPTree<K, V, MAX_BPTREE_NODE_SIZE>::find(const K &x)
{
    if (!_root) {
        return end();
    }
    node_type *cursor = _root;
    while (!cursor->is_leaf) {
        cursor = reinterpret_cast<internal_node_type *>(cursor)->ptr[cursor->child_index_of(x)];
    }
    for (size_t i = 0; i < cursor->size; ++i) {
        if (x == cursor->key[i]) {
            return iterator(reinterpret_cast<leaf_node_type *>(cursor), i);
        }
    }
    return end();
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::const_iterator BPTree<K, V, MAX_BPTREE_NODE_SIZE>::cfind(const K &x) const
{
    if (!_root) {
        return cend();
    }
    node_type *cursor = _root;
    while (!cursor->is_leaf) {
        cursor = reinterpret_cast<internal_node_type *>(cursor)->ptr[cursor->child_index_of(x)];
    }
    for (size_t i = 0; i < cursor->size; ++i) {
        if (x == cursor->key[i]) {
            return const_iterator(reinterpret_cast<leaf_node_type *>(cursor), i);
        }
    }
    return cend();
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::node_type *BPTree<K, V, MAX_BPTREE_NODE_SIZE>::insert_split(const K &x, const V &v, node_type *child, node_type *src, K &split_key)
{
    node_type *new_node = src->is_leaf ? (node_type *)NEW leaf_node_type() : (node_type *)NEW internal_node_type();
    CONTAINER_ASSERT(src->size == MAX_BPTREE_NODE_SIZE);
    size_t i = src->item_index_of(x);
    src->size = (MAX_BPTREE_NODE_SIZE + 1) / 2;
    new_node->size = MAX_BPTREE_NODE_SIZE - (MAX_BPTREE_NODE_SIZE + 1) / 2;

    if (i < src->size) {
        if (!src->is_leaf) {
            split_key = src->key[src->size - 1];
            memcpy(new_node->key, src->key + src->size, sizeof(K) * new_node->size);
            memmove(src->key + i + 1, src->key + i, sizeof(K) * (src->size - i - 1));
            src->key[i] = x;
            node_type **new_ptr = reinterpret_cast<internal_node_type *>(new_node)->ptr;
            node_type **src_ptr = reinterpret_cast<internal_node_type *>(src)->ptr;
            memcpy(new_ptr, src_ptr + src->size, sizeof(node_type *) * (new_node->size + 1));
            memmove(src_ptr + i + 1, src_ptr + i, sizeof(node_type *) * (src->size - i));
            src_ptr[i + 1] = child;
        } else {
            memcpy(new_node->key, src->key + src->size, sizeof(K) * new_node->size);
            memmove(src->key + i + 1, src->key + i, sizeof(K) * (src->size - i));
            src->key[i] = x;
            V *new_val = reinterpret_cast<leaf_node_type *>(new_node)->values;
            V *src_val = reinterpret_cast<leaf_node_type *>(src)->values;
            memcpy(new_val, src_val + src->size, sizeof(V) * new_node->size);
            memmove(src_val + i + 1, src_val + i, sizeof(V) * (src->size - i));
            src_val[i] = v;
            ++src->size;
        }
    } else {
        i -= src->size;
        if (!src->is_leaf) {
            node_type **new_ptr = reinterpret_cast<internal_node_type *>(new_node)->ptr;
            node_type **src_ptr = reinterpret_cast<internal_node_type *>(src)->ptr;
            if (i == 0) {
                split_key = x;
                memcpy(new_node->key, src->key + src->size, sizeof(K) * new_node->size);
                memcpy(new_ptr + 1, src_ptr + src->size + 1, sizeof(node_type *) * (new_node->size));
                new_ptr[i] = child;
            } else {
                split_key = src->key[src->size];
                memcpy(new_ptr, src_ptr + src->size + 1, sizeof(node_type *) * i);
                new_ptr[i] = child;
                memcpy(new_ptr + i + 1, src_ptr + src->size + i + 1, sizeof(node_type *) * (new_node->size - i));
                --i;
                memcpy(new_node->key, src->key + src->size + 1, sizeof(K) * i);
                memcpy(new_node->key + i + 1, src->key + src->size + i + 1, sizeof(K) * (new_node->size - i));
                new_node->key[i] = x;
            }
        } else {
            memcpy(new_node->key, src->key + src->size, sizeof(K) * i);
            memcpy(new_node->key + i + 1, src->key + src->size + i, sizeof(K) * (new_node->size - i));
            new_node->key[i] = x;
            V *new_val = reinterpret_cast<leaf_node_type *>(new_node)->values;
            V *src_val = reinterpret_cast<leaf_node_type *>(src)->values;
            memcpy(new_val, src_val + src->size, sizeof(V) * i);
            new_val[i] = v;
            memcpy(new_val + i + 1, src_val + src->size + i, sizeof(V) * (new_node->size - i));
            ++new_node->size;
        }
    }
    if (src->is_leaf) {
        reinterpret_cast<leaf_node_type *>(new_node)->next = reinterpret_cast<leaf_node_type *>(src)->next;
        reinterpret_cast<leaf_node_type *>(src)->next = reinterpret_cast<leaf_node_type *>(new_node);
        reinterpret_cast<leaf_node_type *>(new_node)->prev = reinterpret_cast<leaf_node_type *>(src);
        if (reinterpret_cast<leaf_node_type *>(new_node)->next) {
            reinterpret_cast<leaf_node_type *>(new_node)->next->prev = reinterpret_cast<leaf_node_type *>(new_node);
        }
        split_key = new_node->key[0];
    }
    return new_node;
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::insert(const K &x, const V &v)
{
    if (!_root) {
        _root = NEW leaf_node_type();
        _root->key[0] = x;
        reinterpret_cast<leaf_node_type *>(_root)->values[0] = v;
        _root->size = 1;
        reinterpret_cast<leaf_node_type *>(_root)->next = NULL;
        reinterpret_cast<leaf_node_type *>(_root)->prev = NULL;
        return;
    }

    node_type *cursor = _root;
    internal_node_type *parent = NULL;
    while (!cursor->is_leaf) {
        parent = reinterpret_cast<internal_node_type *>(cursor);
        cursor = parent->ptr[parent->child_index_of(x)];
    }

    if (cursor->size < MAX_BPTREE_NODE_SIZE) {
        leaf_node_type *leaf = reinterpret_cast<leaf_node_type *>(cursor);
        size_t i = leaf->item_index_of(x);
        memmove(leaf->key + i + 1, leaf->key + i, sizeof(K) * (leaf->size - i));
        leaf->key[i] = x;
        memmove(leaf->values + i + 1, leaf->values + i, sizeof(V) * (leaf->size - i ));
        leaf->values[i] = v;
        ++leaf->size;
        check_invariant(parent);
        return;
    }

    K split_key;
    node_type *new_leaf = insert_split(x, v, NULL, cursor, split_key);
    if (cursor == _root) {
        internal_node_type *new_root = NEW internal_node_type();
        new_root->key[0] = split_key;
        new_root->ptr[0] = cursor;
        new_root->ptr[1] = new_leaf;
        new_root->size = 1;
        _root = new_root;
        check_invariant(_root);
    } else {
        insert_internal(split_key, v, parent, new_leaf);
    }
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::insert_internal(const K &x, const V &v, internal_node_type *cursor, node_type *child)
{
    if (cursor->size < MAX_BPTREE_NODE_SIZE) {
        size_t i = cursor->item_index_of(x);
        memmove(cursor->key + i + 1, cursor->key + i, sizeof(K) * (cursor->size - i));
        cursor->key[i] = x;
        memmove(cursor->ptr + i + 2, cursor->ptr + i + 1, sizeof(node_type *) * (cursor->size - i));
        cursor->ptr[i + 1] = child;
        ++cursor->size;
        check_invariant(cursor);
        return;
    }

    K split_key;
    node_type *new_leaf = insert_split(x, v, child, cursor, split_key);
    if (cursor == _root) {
        internal_node_type *new_root = NEW internal_node_type();
        new_root->key[0] = split_key;
        new_root->ptr[0] = cursor;
        new_root->ptr[1] = new_leaf;
        new_root->size = 1;
        _root = new_root;
        check_invariant(cursor);
    } else {
        insert_internal(split_key, v, find_parent(reinterpret_cast<internal_node_type *>(_root), cursor), new_leaf);
    }
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
typename BPTree<K, V, MAX_BPTREE_NODE_SIZE>::internal_node_type *BPTree<K, V, MAX_BPTREE_NODE_SIZE>::find_parent(internal_node_type *cursor, const node_type *child) const
{
    while (!cursor->is_leaf) {
        internal_node_type *next = reinterpret_cast<internal_node_type *>(cursor->ptr[cursor->child_index_of(child->key[0])]);
        if (next == child) {
            return cursor;
        }
        cursor = next;
    }
    return NULL;
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::remove(iterator it)
{
    leaf_node_type *leaf = reinterpret_cast<leaf_node_type *>(it.node);
    if (leaf == NULL) {
        return;
    }
    size_t pos = it.index;
    --leaf->size;
    memmove(leaf->key + pos, leaf->key + pos + 1, sizeof(K) * (leaf->size - pos));
    memmove(leaf->values + pos, leaf->values + pos + 1, sizeof(V) * (leaf->size - pos));
    if (leaf == _root) {
        if (leaf->size == 0) {
            delete leaf;
            _root = NULL;
        }
        return;
    }

    if (leaf->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2) {
        return;
    }

    auto *parent = find_parent(reinterpret_cast<internal_node_type *>(_root), leaf);
    size_t left_sibling = SIZE_MAX;
    size_t right_sibling = SIZE_MAX;
    for (size_t i = 0; i < parent->size + 1; ++i) {
        if (parent->ptr[i] == leaf) {
            left_sibling = i > 0 ? i - 1 : SIZE_MAX;
            right_sibling = i + 1;
            break;
        }
    }
    if (left_sibling != SIZE_MAX) {
        leaf_node_type *left_node = reinterpret_cast<leaf_node_type *>(parent->ptr[left_sibling]);
        if (left_node->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2 + 1) {
            memmove(leaf->key + 1, leaf->key, sizeof(K) * leaf->size);
            leaf->key[0] = left_node->key[left_node->size - 1];
            memmove(leaf->values + 1, leaf->values, sizeof(V) * leaf->size);
            leaf->values[0] = left_node->values[left_node->size - 1];
            ++leaf->size;
            --left_node->size;
            parent->key[left_sibling] = leaf->key[0];
            check_invariant(parent);
            return;
        }
    }
    if (right_sibling <= parent->size) {
        leaf_node_type *right_node = reinterpret_cast<leaf_node_type *>(parent->ptr[right_sibling]);
        if (right_node->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2 + 1) {
            leaf->key[leaf->size] = right_node->key[0];
            leaf->values[leaf->size] = right_node->values[0];
            ++leaf->size;
            --right_node->size;
            memmove(right_node->key, right_node->key + 1, sizeof(K) * right_node->size);
            memmove(right_node->values, right_node->values + 1, sizeof(V) * right_node->size);
            parent->key[right_sibling - 1] = right_node->key[0];
            check_invariant(parent);
            return;
        }
    }

    if (left_sibling != SIZE_MAX) {
        leaf_node_type *left_node = reinterpret_cast<leaf_node_type *>(parent->ptr[left_sibling]);
        memcpy(left_node->key + left_node->size, leaf->key, sizeof(K) * leaf->size);
        memcpy(left_node->values + left_node->size, leaf->values, sizeof(V) * leaf->size);
        left_node->size += leaf->size;
        reinterpret_cast<leaf_node_type *>(left_node)->next = leaf->next;
        if (leaf->next) {
            leaf->next->prev = left_node;
        }
        remove_internal(parent->key[left_sibling], parent, leaf);
    } else if (right_sibling <= parent->size) {
        leaf_node_type *right_node = reinterpret_cast<leaf_node_type *>(parent->ptr[right_sibling]);
        memcpy(leaf->key + leaf->size, right_node->key, sizeof(K) * right_node->size);
        memcpy(leaf->values + leaf->size, right_node->values, sizeof(V) * right_node->size);
        leaf->size += right_node->size;
        leaf->next = right_node->next;
        if (right_node->next) {
            right_node->next->prev = leaf;
        }
        remove_internal(parent->key[right_sibling - 1], parent, right_node);
    }
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
bool BPTree<K, V, MAX_BPTREE_NODE_SIZE>::remove(const K &x)
{
    auto it = find(x);
    if (it == end()) {
        return false;
    }
    remove(it);
    return true;
}

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::remove_internal(const K &x, internal_node_type *cursor, node_type *child)
{
    if (cursor == _root) {
        if (cursor->size == 1) {
            if (cursor->ptr[1] == child) {
                delete child;
                _root = cursor->ptr[0];
                delete cursor;
                return;
            }
            if (cursor->ptr[0] == child) {
                delete child;
                _root = cursor->ptr[1];
                delete cursor;
                return;
            }
        }
    }
    size_t pos;
    for (pos = 0; pos < cursor->size; pos++) {
        if (cursor->key[pos] == x) {
            break;
        }
    }
    for (size_t i = pos; i < cursor->size; ++i) {
        cursor->key[i] = cursor->key[i + 1];
    }
    for (pos = 0; pos < cursor->size + 1; pos++) {
        if (cursor->ptr[pos] == child) {
            break;
        }
    }
    for (size_t i = pos; i < cursor->size + 1; ++i) {
        cursor->ptr[i] = cursor->ptr[i + 1];
    }
    cursor->size--;
    if (cursor->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2 - 1) {
        return;
    }

    if (cursor==_root) {
        return;
    }

    auto *parent = find_parent(reinterpret_cast<internal_node_type *>(_root), cursor);
    size_t left_sibling = SIZE_MAX;
    size_t right_sibling = SIZE_MAX;
    for (pos = 0; pos < parent->size + 1; pos++) {
        if (parent->ptr[pos] == cursor) {
            left_sibling = pos > 0 ? pos - 1 : SIZE_MAX;
            right_sibling = pos + 1;
            break;
        }
    }

    if (left_sibling != SIZE_MAX) {
        auto *left_node = reinterpret_cast<internal_node_type *>(parent->ptr[left_sibling]);
        if (left_node->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2) {
            memmove(cursor->key + 1, cursor->key, sizeof(K) * cursor->size);
            cursor->key[0] = parent->key[left_sibling];
            parent->key[left_sibling] = left_node->key[left_node->size - 1];
            memmove(cursor->ptr + 1, cursor->ptr, sizeof(node_type *) * (cursor->size + 1));
            cursor->ptr[0] = left_node->ptr[left_node->size];
            ++cursor->size;
            --left_node->size;
            check_invariant(parent);
            return;
        }
    }
    if (right_sibling <= parent->size) {
        auto *right_node = reinterpret_cast<internal_node_type *>(parent->ptr[right_sibling]);
        if (right_node->size >= (MAX_BPTREE_NODE_SIZE + 1) / 2) {
            cursor->key[cursor->size] = parent->key[pos];
            parent->key[pos] = right_node->key[0];
            memmove(right_node->key, right_node->key + 1, sizeof(K) * right_node->size);
            cursor->ptr[cursor->size + 1] = right_node->ptr[0];
            memmove(right_node->ptr, right_node->ptr + 1, sizeof(node_type *) * (right_node->size));
            ++cursor->size;
            --right_node->size;
            check_invariant(parent);
            return;
        }
    }
    if (left_sibling != SIZE_MAX) {
        auto *left_node = reinterpret_cast<internal_node_type *>(parent->ptr[left_sibling]);
        left_node->key[left_node->size] = parent->key[left_sibling];
        memcpy(left_node->key + left_node->size + 1, cursor->key, sizeof(K) * cursor->size);
        memcpy(left_node->ptr + left_node->size + 1, cursor->ptr, sizeof(node_type *) * (cursor->size + 1));
        memset(cursor->ptr, 0, sizeof(node_type *) * (cursor->size + 1));
        left_node->size += cursor->size + 1;
        cursor->size = 0;
        remove_internal(parent->key[left_sibling], parent, cursor);
    } else if (right_sibling <= parent->size) {
        auto *right_node = reinterpret_cast<internal_node_type *>(parent->ptr[right_sibling]);
        cursor->key[cursor->size] = parent->key[right_sibling - 1];
        memcpy(cursor->key + cursor->size + 1, right_node->key, sizeof(K) * right_node->size);
        memcpy(cursor->ptr + cursor->size + 1, right_node->ptr, sizeof(node_type *) * (right_node->size + 1));
        memset(right_node->ptr, 0, sizeof(node_type *) * (right_node->size + 1));
        cursor->size += right_node->size + 1;
        right_node->size = 0;
        remove_internal(parent->key[right_sibling - 1], parent, right_node);
    }
}

#ifdef BTREE_DEBUG
#include <iostream>
using namespace std;
template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::display_internal(const node_type *cursor) const {
    if (cursor != NULL) {
        for (size_t i = 0; i < cursor->size; ++i) {
            cout << cursor->key[i] << " ";
        }
        cout << endl;
        if (!cursor->is_leaf) {
            for (size_t i = 0; i <= cursor->size; ++i) {
                display_internal(reinterpret_cast<const internal_node_type *>(cursor)->ptr[i]);
            }
        }
    }
}
#endif /* BTREE_DEBUG */

template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
void BPTree<K, V, MAX_BPTREE_NODE_SIZE>::clean_up(node_type *cursor)
{
    if (cursor) {
        if (!cursor->is_leaf) {
            for (size_t i = 0; i < cursor->size + 1; ++i) {
                clean_up(reinterpret_cast<internal_node_type *>(cursor)->ptr[i]);
            }
        }
        delete cursor;
    }
}

#ifdef BTREE_VERIFY_DATA
template <typename K, typename V, size_t MAX_BPTREE_NODE_SIZE>
K BPTree<K, V, MAX_BPTREE_NODE_SIZE>::check_invariant(const node_type *cursor) const
{
    if (!cursor) {
        return K();
    }
    if (cursor->is_leaf) {
        return cursor->key[cursor->size - 1];
    }

    
    for (size_t i = 0; i < cursor->size; ++i) {
        const node_type *child = reinterpret_cast<const internal_node_type *>(cursor)->ptr[i];
        K cur_max = check_invariant(child);
        CONTAINER_ASSERT(cur_max < cursor->key[i]);
    }
    K cur_max = check_invariant(reinterpret_cast<const internal_node_type *>(cursor)->ptr[cursor->size]);
    CONTAINER_ASSERT(!(reinterpret_cast<const internal_node_type *>(cursor)->ptr[cursor->size]->key[0] < cursor->key[cursor->size - 1]));
    return cur_max;
}
#endif /* BTREE_VERIFY_DATA */

#endif /* CONTAINER_BTREE_H */
