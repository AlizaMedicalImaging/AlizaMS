/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMSWAPPER_TXX
#define MDCMSWAPPER_TXX

#if defined(_MSC_VER)

#include <cstdlib>
#define bswap_16(X) _byteswap_ushort(X)
#define bswap_32(X) _byteswap_ulong(X)
#define bswap_64(X) _byteswap_uint64(X)

#elif defined(MDCM_HAVE_BYTESWAP_H)

#include <endian.h>
#include <byteswap.h>

#elif defined(__APPLE__)

#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define bswap_16(X) OSSwapInt16(X)
#define bswap_32(X) OSSwapInt32(X)
#define bswap_64(X) OSSwapInt64(X)

#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)

#include <sys/endian.h>
#define bswap_16(X) bswap16(X)
#define bswap_32(X) bswap32(X)
#define bswap_64(X) bswap64(X)

#elif defined(__OpenBSD__)

#include <machine/endian.h>
#define bswap_16(X) swap16(X)
#define bswap_32(X) swap32(X)
#define bswap_64(X) swap64(X)

#elif defined(__MINGW32__)

# define bswap_16(x) ((((x) & 0x00FF) << 8) | \
                      (((x) & 0xFF00) >> 8))
# define bswap_32(x) ((((x) & 0x000000FF) << 24) | \
                      (((x) & 0x0000FF00) << 8)  | \
                      (((x) & 0x00FF0000) >> 8)  | \
                      (((x) & 0xFF000000) >> 24))
# define bswap_64(x) ((((x) & 0x00000000000000FFULL) << 56) | \
                      (((x) & 0x000000000000FF00ULL) << 40) | \
                      (((x) & 0x0000000000FF0000ULL) << 24) | \
                      (((x) & 0x00000000FF000000ULL) << 8)  | \
                      (((x) & 0x000000FF00000000ULL) >> 8)  | \
                      (((x) & 0x0000FF0000000000ULL) >> 24) | \
                      (((x) & 0x00FF000000000000ULL) >> 40) | \
                      (((x) & 0xFF00000000000000ULL) >> 56))

#else

// If not, test whether the workaround for __MINGW32__ can be used for this platform.
#error "Byte swap methods are not available."

#endif

#include "mdcmTag.h"

namespace mdcm
{

#ifdef MDCM_WORDS_BIGENDIAN
  template <> inline uint16_t SwapperNoOp::Swap<uint16_t>(uint16_t val)
  {
    return bswap_16(val);
  }

  template <> inline int16_t SwapperNoOp::Swap<int16_t>(int16_t val)
  {
    return Swap((uint16_t)val);
  }

  template <> inline uint32_t SwapperNoOp::Swap<uint32_t>(uint32_t val)
  {
    return bswap_32(val);
  }

  template <> inline int32_t SwapperNoOp::Swap<int32_t>(int32_t val)
  {
    return Swap((uint32_t)val);
  }

  template <> inline float SwapperNoOp::Swap<float>(float val)
  {
    return Swap((uint32_t)val);
  }

  template <> inline uint64_t SwapperNoOp::Swap<uint64_t>(uint64_t val)
  {
    return bswap_64(val);
  }

  template <> inline int64_t SwapperNoOp::Swap<int64_t>(int64_t val)
  {
    return Swap((uint64_t)val);
  }

  template <> inline double SwapperNoOp::Swap<double>(double val)
  {
    return Swap((uint64_t)val);
  }

  template <> inline Tag SwapperNoOp::Swap<Tag>(Tag val)
  {
    return Tag(Swap(val.GetGroup()), Swap(val.GetElement()));
  }

  template <> inline void SwapperNoOp::SwapArray(uint8_t *, unsigned int) {}

  template <> inline void SwapperNoOp::SwapArray(float *array, unsigned int n)
  {
    switch(sizeof(float))
    {
      case 4:
        SwapperNoOp::SwapArray<uint32_t>((uint32_t*)array,n);
        break;
      default:
        assert(0);
    }
  }

  template <> inline void SwapperNoOp::SwapArray(double *array, unsigned int n)
  {
    switch(sizeof(double))
    {
      case 8:
        SwapperNoOp::SwapArray<uint64_t>((uint64_t*)array,n);
        break;
      default:
        assert(0);
    }
  }

#else

  template <> inline uint16_t SwapperDoOp::Swap<uint16_t>(uint16_t val)
  {
    return bswap_16(val);
  }
  template <> inline int16_t SwapperDoOp::Swap<int16_t>(int16_t val)
  {
    return Swap((uint16_t)val);
  }

  template <> inline uint32_t SwapperDoOp::Swap<uint32_t>(uint32_t val)
  {
    return bswap_32(val);
  }

  template <> inline int32_t SwapperDoOp::Swap<int32_t>(int32_t val)
  {
    return Swap((uint32_t)val);
  }

  template <> inline float SwapperDoOp::Swap<float>(float val)
  {
    return static_cast<float>(Swap((uint32_t)val));
  }

  template <> inline uint64_t SwapperDoOp::Swap<uint64_t>(uint64_t val)
  {
    return bswap_64(val);
  }

  template <> inline int64_t SwapperDoOp::Swap<int64_t>(int64_t val)
  {
    return Swap((uint64_t)val);
  }

  template <> inline double SwapperDoOp::Swap<double>(double val)
  {
    return static_cast<double>(Swap((uint64_t)val));
  }

  template <> inline Tag SwapperDoOp::Swap<Tag>(Tag val)
  {
    return Tag(Swap((uint32_t)val.GetElementTag()));
  }

  template <> inline void SwapperDoOp::SwapArray(uint8_t *, size_t) {}

  template <> inline void SwapperDoOp::SwapArray(float *array, size_t n)
  {
    switch(sizeof(float))
    {
      case 4:
        SwapperDoOp::SwapArray<uint32_t>((uint32_t*)array,n);
        break;
      default:
        assert(0);
    }
  }

  template <> inline void SwapperDoOp::SwapArray(double *array, size_t n)
  {
    switch(sizeof(double))
    {
      case 8:
        SwapperDoOp::SwapArray<uint64_t>((uint64_t*)array,n);
        break;
      default:
        assert(0);
    }
  }

#endif

} // end namespace mdcm

#endif // MDCMSWAPPER_TXX
