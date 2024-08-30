/**
 * Copyright © 2024 Mingwei Huang
 * A mimic to boost::icl::interval_set
 */

#ifndef CONTAINER_INTERVAL_H
#define CONTAINER_INTERVAL_H

namespace mem_container {
template <typename T>
struct DiscreteInterval {
    T start{};
    T end{};

    struct DiscreteIntervalPair {
        DiscreteInterval left_ival;
        DiscreteInterval right_ival;
    };

    struct Bound {
        T val;

        inline bool operator<(const Bound &other) const { return val < other.val; }
        inline bool operator==(const Bound &other) const { return val == other.val; }
    };
    inline Bound get_left_bound() const { return Bound{start}; }
    inline Bound get_right_bound() const { return Bound{end}; }

    DiscreteInterval() {}
    DiscreteInterval(const T &val) : start(val), end(val) {}
    DiscreteInterval(const Bound &left_bound, const Bound &right_bound) : start(left_bound.val), end(right_bound.val) {}
    DiscreteInterval(const T &start, const T &end) : start(start), end(end) {}
    DiscreteInterval operator&(const DiscreteInterval &other) const
    {
        return DiscreteInterval(std::max(start, other.start), std::min(end, other.end));
    }
    DiscreteInterval operator|(const DiscreteInterval &other) const
    {
        return DiscreteInterval(std::min(start, other.start), std::max(end, other.end));
    }
    DiscreteIntervalPair operator^(const DiscreteInterval &other) const
    {
        DiscreteIntervalPair res;
        if (start < other.start) {
            res.left_ival.start = start;
            res.left_ival.end = other.start;
        } else {
            res.left_ival.start = other.start;
            res.left_ival.end = start;
        }

        if (end < other.end) {
            res.right_ival.start = end;
            res.right_ival.end = other.end;
        } else {
            res.right_ival.start = other.end;
            res.right_ival.end = end;
        }

        return res;
    }
    inline DiscreteInterval &operator&=(const DiscreteInterval &other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this & other;
        return *this;
    }
    inline DiscreteInterval &operator&=(DiscreteInterval &&other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this & std::forward(other);
        return *this;
    }
    inline DiscreteInterval &operator|=(const DiscreteInterval &other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this | other;
        return *this;
    }
    inline DiscreteInterval &operator|=(DiscreteInterval &&other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this | other;
        return *this;
    }
    inline bool operator==(const DiscreteInterval &other) const { return start == other.start && end == other.end; }
    inline bool empty() const { return start > end; }
    inline bool contains(const DiscreteInterval &other) const { return start <= other.start && end >= other.end; }
    inline bool touch(const DiscreteInterval &other) const { return start <= other.end && end >= other.start; }
};

template <typename T>
struct ContinuousInterval {
    bool start_open{false};
    bool end_open{false};
    T start{};
    T end{};

    struct ContinuousIntervalPair {
        ContinuousInterval left_ival;
        ContinuousInterval right_ival;
    };

    struct Bound {
        T val;
        bool open;

        inline bool operator<(const Bound &other) const { return val < other.val || (val == other.val && open && !other.open); }
        inline bool operator==(const Bound &other) const { return val == other.val && open == other.open; }
    };
    inline Bound get_left_bound() const { return Bound{start, start_open}; }
    inline Bound get_right_bound() const { return Bound{end, end_open}; }

    ContinuousInterval() {}
    /* intensionally implicit constructor */
    ContinuousInterval(const T &val) : start_open(false), end_open(false), start(val), end(val) {}
    ContinuousInterval(const Bound &left_bound, const Bound &right_bound)
        : start_open(left_bound.open), end_open(right_bound.open), start(left_bound.val), end(right_bound.val) {}
    ContinuousInterval(const T &start, const T &end, bool start_open = false, bool end_open = false)
        : start_open(start_open), end_open(end_open), start(start), end(end) {}

    static ContinuousInterval right_open(T start, T end) { return ContinuousInterval(start, end, false, true); }
    static ContinuousInterval open(T start, T end) { return ContinuousInterval(start, end, true, true); }
    static ContinuousInterval closed(T start, T end) { return ContinuousInterval(start, end, false, false); }
    ContinuousInterval operator&(const ContinuousInterval &other) const
    {
        ContinuousInterval res;
        if (start < other.start) {
            res.start = other.start;
            res.start_open = other.start_open;
        } else if (start == other.start) {
            res.start = start;
            res.start_open = start_open && other.start_open;
        } else {
            res.start = start;
            res.start_open = start_open;
        }

        if (end < other.end) {
            res.end = end;
            res.end_open = end_open;
        } else if (end == other.end) {
            res.end = end;
            res.end_open = end_open && other.end_open;
        } else {
            res.end = other.end;
            res.end_open = other.end_open;
        }

        return res;
    }
    ContinuousInterval operator|(const ContinuousInterval &other) const
    {
        ContinuousInterval res;
        if (start < other.start) {
            res.start = start;
            res.start_open = start_open;
        } else if (start == other.start) {
            res.start = start;
            res.start_open = start_open || other.start_open;
        } else {
            res.start = other.start;
            res.start_open = other.start_open;
        }

        if (end < other.end) {
            res.end = other.end;
            res.end_open = other.end_open;
        } else if (end == other.end) {
            res.end = end;
            res.end_open = end_open || other.end_open;
        } else {
            res.end = end;
            res.end_open = end_open;
        }

        return res;
    }
    ContinuousIntervalPair operator^(const ContinuousInterval &other) const
    {
        ContinuousIntervalPair res;
        if (start < other.start) {
            res.left_ival.start = start;
            res.left_ival.start_open = start_open;
            res.left_ival.end = other.start;
            res.left_ival.end_open = !other.start_open;
        } else if (start == other.start) {
            res.left_ival.start = start;
            res.left_ival.start_open = false;
            res.left_ival.end = start;
            res.left_ival.end_open = start_open ^ other.start_open;
        } else {
            res.left_ival.start = other.start;
            res.left_ival.start_open = !other.start_open;
            res.left_ival.end = start;
            res.left_ival.end_open = start_open;
        }

        if (end < other.end) {
            res.right_ival.start = end;
            res.right_ival.start_open = end_open;
            res.right_ival.end = other.end;
            res.right_ival.end_open = !other.end_open;
        } else if (end == other.end) {
            res.right_ival.start = end;
            res.right_ival.start_open = false;
            res.right_ival.end = end;
            res.right_ival.end_open = end_open ^ other.end_open;
        } else {
            res.right_ival.start = other.end;
            res.right_ival.start_open = !other.end_open;
            res.right_ival.end = end;
            res.right_ival.end_open = end_open;
        }

        return res;
    }
    inline ContinuousInterval &operator&=(const ContinuousInterval &other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this & other;
        return *this;
    }
    inline ContinuousInterval &operator&=(ContinuousInterval &&other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this & std::forward(other);
        return *this;
    }
    inline ContinuousInterval &operator|=(const ContinuousInterval &other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this | other;
        return *this;
    }
    inline ContinuousInterval &operator|=(ContinuousInterval &&other)
    {
        /* hopefully compiler is smart enough to optimize it... */
        *this = *this | other;
        return *this;
    }
    /* we don't handle [5,5] == [5,6) for integral types, it's hard to guess the base unit (and use ClosedInterval for that case...) */
    inline bool operator==(const ContinuousInterval &other) const
    {
        return start == other.start && end == other.end && start_open == other.start_open && end_open == other.end_open;
    }
    inline bool empty() const { return start > end || (start == end && (start_open || end_open)); }
    inline bool contains(const ContinuousInterval &other) const
    {
        return start <= other.start && end >= other.end &&
            (start < other.start || (start == other.start && (!start_open || other.start_open))) &&
            (end > other.end || (end == other.end && (!end_open || other.end_open)));
    }
    inline bool touch(const ContinuousInterval &other) const
    {
        return (start < other.end && end > other.start) || (start == other.end && (!start_open || !end_open)) ||
               (end == other.start && (!end_open || !start_open));
    }
};
} /* namespace mem_container */

#endif /* CONTAINER_INTERVAL_H */
