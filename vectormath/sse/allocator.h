/*
 * (C) mihail.isakov@googlemail.com 2009-2022
 */

#ifndef VECTORMATH_ALIGNED_ALLOCATOR_H
#define VECTORMATH_ALIGNED_ALLOCATOR_H

// clang-format off

///////////////////////////////////////////////////////
//
//
//
//
#define VECTORMATH_CPP17_ALIGNED_ALLOC 0
#define VECTORMATH_DEBUG_PRINT 0
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
#if !(defined VECTORMATH_CPP17_ALIGNED_ALLOC && VECTORMATH_CPP17_ALIGNED_ALLOC == 1)
#if !(defined _MSC_VER && _MSC_VER >= 1400)
#include <cstdint>
#endif
#endif
#if (defined VECTORMATH_DEBUG_PRINT && VECTORMATH_DEBUG_PRINT == 1)
#include <cstddef>
#include <iostream>
#endif

// A 'size' must be > 0, the behavior is undefined if 'alignment' is
// not a power of two.
inline void * vectormathAlignedAlloc(size_t size, size_t alignment)
{
#if (defined VECTORMATH_DEBUG_PRINT && VECTORMATH_DEBUG_PRINT == 1)
// The default system alignment should be the size of the largest scalar,
// usually 8 or 16.
  std::cout << "vectormathAlignedAlloc(" << size << ", " << alignment
            << "), default alignment is " << alignof(std::max_align_t)
            << std::endl;
#endif
#if (defined VECTORMATH_CPP17_ALIGNED_ALLOC && VECTORMATH_CPP17_ALIGNED_ALLOC == 1)
  return std::aligned_alloc(alignment, size);
#else
#if (defined _MSC_VER && _MSC_VER >= 1400)
  return _aligned_malloc(size, alignment);
#else
  const uintptr_t tmp0 = alignment - 1;
  const size_t    tmp1 = sizeof(void*) + tmp0;
  void * p = std::malloc(size + tmp1);
  if (!p) return nullptr;
  uintptr_t tmp2 = reinterpret_cast<uintptr_t>(p) + tmp1;
  tmp2 &= (~tmp0);
  void * ptr = reinterpret_cast<void*>(tmp2);
  *(reinterpret_cast<void**>(ptr) - 1) = p;
  return ptr;
#endif
#endif
}

inline void vectormathAlignedFree(void * ptr)
{
#if (defined VECTORMATH_DEBUG_PRINT && VECTORMATH_DEBUG_PRINT == 1)
  std::cout << "vectormathAlignedFree(" << reinterpret_cast<uintptr_t>(ptr) << ")"
            << std::endl;
#endif
#if (defined VECTORMATH_CPP17_ALIGNED_ALLOC && VECTORMATH_CPP17_ALIGNED_ALLOC == 1)
// The 'not null' verification should be not required.
  if (ptr)
  {
    std::free(ptr);
  }
#else
#if (defined _MSC_VER && _MSC_VER >= 1400)
// The 'not null' verification should be not required.
  if (ptr)
  {
    _aligned_free(ptr);
  }
#else
// The 'not null' verification is required.
  if (ptr)
  {
    std::free(*(reinterpret_cast<void**>(ptr) - 1));
  }
#endif
#endif
}

/*
 *
 *   Note: C++17 has operators 'new' with 'std::align_val_t' argument, e.g.
 * void * operator new  (std::size_t count, std::align_val_t al);
 * void * operator new[](std::size_t count, std::align_val_t al);
 * etc.
 *
 *
 *   Placement 'new'/'delete' do the same as in the standard implementation,
 * 'new' simply returns pointer, 'delete' does nothing.
 *
 */

#define VECTORMATH_ALIGNED16_NEW()                              \
void * operator new(size_t b)                                   \
{                                                               \
  void * p = vectormathAlignedAlloc(b, 16);                     \
  if (!p)                                                       \
  {                                                             \
    throw std::bad_alloc();                                     \
  }                                                             \
  return p;                                                     \
}                                                               \
void * operator new[](size_t b)                                 \
{                                                               \
  void * p = vectormathAlignedAlloc(b, 16);                     \
  if (!p)                                                       \
  {                                                             \
    throw std::bad_alloc();                                     \
  }                                                             \
  return p;                                                     \
}                                                               \
void * operator new(size_t b, const std::nothrow_t&) noexcept   \
{                                                               \
  return vectormathAlignedAlloc(b, 16);                         \
}                                                               \
void * operator new[](size_t b, const std::nothrow_t&) noexcept \
{                                                               \
  return vectormathAlignedAlloc(b, 16);                         \
}                                                               \
void operator delete(void * p) noexcept                         \
{                                                               \
  vectormathAlignedFree(p);                                     \
}                                                               \
void operator delete[](void * p) noexcept                       \
{                                                               \
  vectormathAlignedFree(p);                                     \
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

#endif // VECTORMATH_ALIGNED_ALLOCATOR_H
