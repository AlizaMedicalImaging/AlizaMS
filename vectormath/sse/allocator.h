/*
 * (C) mihail.isakov@googlemail.com
 */

#ifndef ALIGNED_ALLOCATOR__H
#define ALIGNED_ALLOCATOR__H

// clang-format off

///////////////////////////////////////////////////////
//
//
//
//
#define VECTORMATH_USE_CPP17_ALIGNED_ALLOC 0
#define VECTORMATH_PRINT 0
//
//
//
//
///////////////////////////////////////////////////////

#if defined(_MSC_VER)
// Visual Studio
#define VECTORMATH_ALIGNED(a) __declspec(align(16)) a
#define VECTORMATH_ALIGNED_PRE  __declspec(align(16))
#define VECTORMATH_ALIGNED_POST
#elif defined(__GNUC__)
// GCC or Clang
#define VECTORMATH_ALIGNED(a) a __attribute__((aligned(16)))
#define VECTORMATH_ALIGNED_PRE
#define VECTORMATH_ALIGNED_POST __attribute__((aligned(16)))
#else
#error "Define alignment for the compiler"
#endif

#include <cstdlib>
#include <new>
#if !(defined VECTORMATH_USE_CPP17_ALIGNED_ALLOC && VECTORMATH_USE_CPP17_ALIGNED_ALLOC == 1)
#if !(defined _MSC_VER && _MSC_VER >= 1400)
#include <cstdint>
#endif
#endif
#if (defined VECTORMATH_PRINT && VECTORMATH_PRINT == 1)
#include <iostream>
#endif

// The behavior is undefined if alignment is not a power of two
static void * aligned__alloc__(size_t size, size_t alignment)
{
#if (defined VECTORMATH_PRINT && VECTORMATH_PRINT == 1)
  std::cout << "aligned__alloc__(" << size << ", " << alignment << ")" << std::endl;
#endif
#if (defined VECTORMATH_USE_CPP17_ALIGNED_ALLOC && VECTORMATH_USE_CPP17_ALIGNED_ALLOC == 1)
  return std::aligned_alloc(alignment, size);
#else
#if (defined _MSC_VER && _MSC_VER >= 1400)
  return _aligned_malloc(size, alignment);
#else
  const uintptr_t tmp0 = alignment - 1;
  const size_t    tmp1 = sizeof(void*) + tmp0;
  void * p = malloc(size + tmp1);
  if (!p) return nullptr;
  uintptr_t tmp2 = (uintptr_t)p + tmp1;
  tmp2 &= (~tmp0);
  void * ptr = (void*)tmp2;
  *((void**)ptr - 1) = p;
  return ptr;
#endif
#endif
}

static void aligned__free__(void * ptr)
{
#if (defined VECTORMATH_PRINT && VECTORMATH_PRINT == 1)
  std::cout << "aligned__free__" << std::endl;
#endif
#if (defined VECTORMATH_USE_CPP17_ALIGNED_ALLOC && VECTORMATH_USE_CPP17_ALIGNED_ALLOC == 1)
  if (ptr) std::free(ptr);
#else
#if (defined _MSC_VER && _MSC_VER >= 1400)
  if (ptr) _aligned_free(ptr);
#else
  if (ptr) free(*((void**)ptr - 1));
#endif
#endif
}

/*
 * Note: C++17 has new operators with appropriate arguments, e.g.
 *
 * void* operator new  (std::size_t count, std::align_val_t al);
 * void* operator new[](std::size_t count, std::align_val_t al);
 * void* operator new  (std::size_t count, std::align_val_t al, const std::nothrow_t&);
 * void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t&);
 *
 */

#define VECTORMATH_ALIGNED16_NEW()                              \
void * operator new(size_t b)                                   \
{                                                               \
  void * p = aligned__alloc__(b, 16);                           \
  if (!p) throw std::bad_alloc();                               \
  return p;                                                     \
}                                                               \
void * operator new[](size_t b)                                 \
{                                                               \
  void * p = aligned__alloc__(b, 16);                           \
  if (!p) throw std::bad_alloc();                               \
  return p;                                                     \
}                                                               \
void * operator new(size_t b, const std::nothrow_t&) noexcept   \
{                                                               \
  return aligned__alloc__(b, 16);                               \
}                                                               \
void * operator new[](size_t b, const std::nothrow_t&) noexcept \
{                                                               \
  return aligned__alloc__(b, 16);                               \
}                                                               \
void operator delete(void * p) noexcept                         \
{                                                               \
  aligned__free__(p);                                           \
}                                                               \
void operator delete[](void * p) noexcept                       \
{                                                               \
  aligned__free__(p);                                           \
}                                                               \
void * operator new(size_t, void * p) noexcept                  \
{                                                               \
  return p;                                                     \
}                                                               \
void * operator new[](size_t, void * p) noexcept                \
{                                                               \
  return p;                                                     \
}                                                               \
void operator delete(void *, void *) noexcept {}                \
void operator delete[](void *, void *) noexcept {}

// clang-format off

#endif // ALIGNED_ALLOCATOR__H
