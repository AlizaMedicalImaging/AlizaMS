#ifndef ALIGNED_ALLOCATOR__H
#define ALIGNED_ALLOCATOR__H

#include <cstdlib>
#if defined _MSC_VER
#define ALIGN16(a) __declspec(align(16)) a
#define ALIGN16_PRE __declspec(align(16))
#define ALIGN16_POST
#else
#define ALIGN16(a) a __attribute__((aligned(16)))
#define ALIGN16_PRE
#define ALIGN16_POST __attribute__((aligned(16)))
#endif

#if !(defined _MSC_VER && _MSC_VER >= 1400)
#include <cstdint>
#endif

static void * aligned__alloc__(size_t size, size_t alignment)
{
#if defined _MSC_VER && _MSC_VER >= 1400
  return _aligned_malloc(size, alignment);
#else
  const uintptr_t tmp0 = alignment - 1;
  const size_t    tmp1 = sizeof(void*) + tmp0;
  void * p = malloc(size + tmp1);
  if(!p) return NULL;
  uintptr_t tmp2 = (uintptr_t)p + tmp1;
  tmp2 &= (~tmp0);
  void * ptr = (void*)tmp2;
  *((void**)ptr - 1) = p;
  return ptr;
#endif
}

static void aligned__free__(void * ptr)
{
#if defined _MSC_VER && _MSC_VER >= 1400
  if (ptr) _aligned_free(ptr);
#else
  if (ptr) free(*((void**)ptr - 1));
#endif
}

#define MY_DECLARE_NEW() \
inline void* operator new(size_t bytes) {return aligned__alloc__(bytes,16);} \
inline void  operator delete(void* ptr) {aligned__free__(ptr);} \
inline void* operator new(size_t, void* ptr) {return ptr; } \
inline void  operator delete(void*, void*) {} \
inline void* operator new[](size_t bytes) {return aligned__alloc__(bytes,16);} \
inline void  operator delete[](void* ptr) {aligned__free__(ptr);} \
inline void* operator new[](size_t, void* ptr) {return ptr;} \
inline void  operator delete[](void*, void*) {}

#endif // ALIGNED_ALLOCATOR__H

