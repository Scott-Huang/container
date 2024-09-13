/**
 * CopyRight © 2024 Mingwei Huang
 * Definition for container
 */

#ifndef CONTAINER_DEFINITION_H
#define CONTAINER_DEFINITION_H

#define CONTAINER_USE_STL false
#define CONTAINER_USE_BOOST false
#define CONTAINER_USE_POSTGRES_MMGR false
#define CONTAINER_DEBUG_LEVEL 0

#if CONTAINER_USE_POSTGRES_MMGR
#include "utils/palloc.h"
#define MemCxtHolder MemoryContext _cxt{NULL};
#define NEW New(_cxt)
#define UseMemCxt() (void)0
#define ResetMemCxt() (void)0
#define CreateMemCxt() (void)0
#define CreateSharedMemCxt() (void)0
#define DestroyMemCxt() (void)0
#define ExchangeMemCxt(other) (void)0
#define malloc palloc
#define free pfree
#define realloc repalloc
#define calloc palloc0
#define NO_DESTROYER 1
#else
#define MemCxtHolder
#define UseMemCxt() (void)0
#define ResetMemCxt() (void)0
#define CreateMemCxt() (void)0
#define CreateSharedMemCxt() (void)0
#define DestroyMemCxt() (void)0
#define ExchangeMemCxt(other) (void)0
#define New(ctx) new
#define NEW new
struct BaseObject {};
#endif /* CONTAINER_USE_POSTGRES_MMGR */

#if CONTAINER_DEBUG_LEVEL >= 1
#include <cassert>
#define CONTAINER_ASSERT(x) assert(x)
#define CONTAINER_BPLUSTREE_DEBUG 1
#define CONTAINER_INTERVAL_DEBUG 1
#define CONTAINER_VECTOR_DEBUG 1
#else
#define CONTAINER_ASSERT(x)
#endif /* CONTAINER_DEBUG_LEVEL */

#ifdef CONTAINER_VECTOR_DEBUG
#define CONTAINER_VECTOR_VEIFY_DATA 1
#if CONTAINER_DEBUG_LEVEL > 1
#define CONTAINER_VECTOR_VERIFY_DATA_STRONG 1
#endif /* CONTAINER_DEBUG_LEVEL */
#endif /* CONTAINER_VECTOR_DEBUG */

#ifdef CONTAINER_BPLUSTREE_DEBUG
#if CONTAINER_DEBUG_LEVEL > 1
#define CONTAINER_BPLUSTREE_VERIFY_DATA 1
#endif /* CONTAINER_DEBUG_LEVEL */
#endif /* CONTAINER_BPLUSTREE_DEBUG */

namespace container_helper {
template <typename>
constexpr std::false_type has_destroyer_h (long);
template <typename T>
constexpr auto has_destroyer_h (int) -> decltype( std::declval<T>().destroy(), std::true_type{} );
template <typename T>
using has_destroyer = decltype( has_destroyer_h<T>(0) );
template <typename T>
void optional_destroy(T &obj, std::true_type const &) { obj.destroy(); }
template <typename T>
void optional_destroy(T &, std::false_type const &) {}
} /* namespace container_helper */

/* call destroy if the object has a destroy() function */
template <typename T>
void optional_destroy(T &obj) { container_helper::optional_destroy(obj, container_helper::has_destroyer<T>{}); }

struct EmptyObject {};

#endif /* CONTAINER_DEFINITION_H */
