#ifndef ALIGNED_ALLOCATOR__H
#define ALIGNED_ALLOCATOR__H

#include <stdlib.h>
#if defined _MSC_VER
#define ALIGN16(a) __declspec(align(16)) a
#define ALIGN16_PRE __declspec(align(16))
#define ALIGN16_POST
#else
#include <stdint.h>
#define ALIGN16(a) a __attribute__((aligned(16)))
#define ALIGN16_PRE
#define ALIGN16_POST __attribute__((aligned(16)))
#endif

#if !(defined _MSC_VER && _MSC_VER >= 1400)
static char * align__pointer__(char * p, size_t x) // x = alignment - 1
{
  struct PtrSizeT
  {
    union
    {
      char * k;
      uintptr_t i;
    };
  };
  PtrSizeT j;
  j.k = p;
  j.i += x;
  j.i &= (~x);
  return j.k;
}
#endif

static void * aligned__alloc__(size_t size, size_t alignment)
{
#if defined _MSC_VER && _MSC_VER >= 1400
  return _aligned_malloc(size, alignment);
#else
  const size_t v = sizeof(void*);
  const size_t x = alignment - 1;
  char * p = (char*)malloc(size + v + x);
  if(!p) return NULL;
  void * ptr = (void*)align__pointer__(p + v, x);
  *((void**)ptr - 1) = (void*)p;
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


