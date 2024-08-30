/**
 * Copyright © 2024 Mingwei Huang
 * A mimic to boost::icl::interval_set
 */

#ifndef CONTAINER_INTERVAL_SET_H
#define CONTAINER_INTERVAL_SET_H

#include <utility>
#include "interval.h"
#include "../definition.h"
#include "../bptree/btree.h"
#include "../vector/vector.h"

namespace mem_container {
template <typename T, template<typename> class IntervalType = ContinuousInterval>
class IntervalSet {
public:
    using interval_type = IntervalType<T>;
    using Bound = interval_type::Bound;
#if CONTAINER_USE_STL
    using bound_vector_type = std::vector<Bound>;
#else
    using bound_vector_type = Vector<Bound>;
#endif /* CONTAINER_USE_STL */

    IntervalSet() {}
    IntervalSet(const IntervalSet &other) = delete;
    IntervalSet operator=(const IntervalSet &other) = delete;
    IntervalSet(IntervalSet &&other)
    {
        _ivals = std::move(other._ivals);
    }
    IntervalSet operator=(IntervalSet &&other)
    {
        if (this != &other) {
            _ivals = std::move(other._ivals);
        }
        return *this;
    }
#ifdef NO_DESTROYER
    ~IntervalSet() {}
#else
    ~IntervalSet() = default;
#endif /* NO_DESTROYER */
    void insert(const interval_type &ival);
    bool contains(const interval_type &ival) const;
    void destroy() { _ivals.destroy(); }
    size_t iterative_size() const { return _ivals.size(); }
private:
    BPTree<Bound, Bound> _ivals{};
};
} /* namespace mem_container */

/* place for implementation */

using namespace mem_container;

template <typename T, template<typename> class IntervalType>
void IntervalSet<T, IntervalType>::insert(const interval_type &ival)
{
    auto it = _ivals.cfind_left(ival.get_left_bound());
    if (it == _ivals.cend()) {
        it = _ivals.cbegin();
    } else if (interval_type(it.key(), it.value()).contains(ival)) {
        return;
    }
    bound_vector_type to_delete;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    Bound last_end;
    for (; it != _ivals.cend(); ++it) {
        if (ival.touch(interval_type(it.key(), it.value()))) {
            to_delete.push_back(it.key());
            last_end = it.value();
        }
        if (ival.get_right_bound() < it.key()) {
            break;
        }
    }
    if (to_delete.empty()) {
        _ivals.insert(ival.get_left_bound(), ival.get_right_bound());
        return;
    }

    interval_type new_ival = ival | interval_type(to_delete.front(), last_end);
    for (const auto &bound : to_delete) {
#ifdef CONTAINER_INTERVAL_DEBUG
        bool res = _ivals.remove(bound);
        CONTAINER_ASSERT(res);
#else
        _ivals.remove(bound);
#endif /* CONTAINER_INTERVAL_DEBUG */
    }
    optional_destroy(to_delete);
    _ivals.insert(new_ival.get_left_bound(), new_ival.get_right_bound());
#pragma GCC diagnostic pop /* -Wmaybe-uninitialized */
}

template <typename T, template<typename> class IntervalType>
bool IntervalSet<T, IntervalType>::contains(const interval_type &ival) const
{
    auto it = _ivals.cfind_left(ival.get_left_bound());
    if (it == _ivals.cend()) {
        return false;
    }
    interval_type cur_ival = interval_type(it.key(), it.value());
    return cur_ival.contains(ival);
}

#endif /* CONTAINER_INTERVAL_SET_H */
