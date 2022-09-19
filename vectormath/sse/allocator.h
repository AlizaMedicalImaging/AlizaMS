/*
 * (C) mihail.isakov@googlemail.com
 */

#ifndef ALIGNED_ALLOCATOR__H
#define ALIGNED_ALLOCATOR__H

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

#define VECTORMATH_ALIGNED16_NEW() \
inline void* operator new(size_t bytes) noexcept { return aligned__alloc__(bytes,16); } \
inline void  operator delete(void* ptr) noexcept { aligned__free__(ptr); } \
inline void* operator new(size_t, void* ptr) noexcept { return ptr; } \
inline void  operator delete(void*, void*) noexcept {} \
inline void* operator new[](size_t bytes) noexcept { return aligned__alloc__(bytes,16); } \
inline void  operator delete[](void* ptr) noexcept { aligned__free__(ptr); } \
inline void* operator new[](size_t, void* ptr) noexcept { return ptr; } \
inline void  operator delete[](void*, void*) noexcept {}

#endif // ALIGNED_ALLOCATOR__H
