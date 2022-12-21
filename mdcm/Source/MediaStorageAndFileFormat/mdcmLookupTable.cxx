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

#include "mdcmLookupTable.h"
#include "mdcmTrace.h"
#include <vector>
#include <set>
#include <climits>
#include <cstring>
#include <cstdint>

namespace mdcm
{

class LookupTableInternal
{
public:
  LookupTableInternal()
    : RGB()
  {
    Length[0] = Length[1] = Length[2] = 0;
    Subscript[0] = Subscript[1] = Subscript[2] = 0;
    BitSize[0] = BitSize[1] = BitSize[2] = 0;
  }
  unsigned int               Length[3];
  unsigned short             Subscript[3];
  unsigned short             BitSize[3];
  std::vector<unsigned char> RGB;
};

LookupTable::LookupTable()
{
  Internal = new LookupTableInternal;
  BitSample = 0;
  IncompleteLUT = false;
}

LookupTable::~LookupTable()
{
  delete Internal;
}

bool
LookupTable::Initialized() const
{
  bool b1 = BitSample != 0;
  bool b2 = Internal->BitSize[0] != 0 && Internal->BitSize[1] != 0 && Internal->BitSize[2] != 0;
  return b1 && b2;
}

void
LookupTable::Clear()
{
  BitSample = 0;
  IncompleteLUT = false;
  delete Internal;
  Internal = new LookupTableInternal;
}

void
LookupTable::Allocate(unsigned short bitsample)
{
  if (bitsample == 8)
  {
    Internal->RGB.resize(256 * 3);
  }
  else if (bitsample == 16)
  {
    Internal->RGB.resize(65536 * 2 * 3);
  }
  else
  {
    assert(0);
  }
  BitSample = bitsample;
}

void
LookupTable::InitializeLUT(LookupTableType type,
                           unsigned short  length,
                           unsigned short  subscript,
                           unsigned short  bitsize)
{
  if (bitsize != 8 && bitsize != 16)
  {
    return;
  }
  assert(bitsize == 8 || bitsize == 16);
  if (length == 0)
  {
    Internal->Length[type] = 65536;
  }
  else
  {
    if (length != 256)
    {
      IncompleteLUT = true;
    }
    Internal->Length[type] = length;
  }
  Internal->Subscript[type] = subscript;
  Internal->BitSize[type] = bitsize;
}

unsigned int
LookupTable::GetLUTLength(LookupTableType type) const
{
  return Internal->Length[type];
}

void
LookupTable::SetLUT(LookupTableType enhanced_type, const unsigned char * array, unsigned int length)
{
  (void)length;
  // Supplemental SUPPLRED = 4
  LookupTableType type = enhanced_type;
  if (type == SUPPLRED)
  {
    type = RED;
  }
  else if (type == SUPPLGREEN)
  {
    type = GREEN;
  }
  else if (type == SUPPLBLUE)
  {
    type = BLUE;
  }
  if (!Internal->Length[type])
  {
    mdcmDebugMacro("Need to set length first");
    return;
  }
  if (!IncompleteLUT)
  {
    assert(Internal->RGB.size() == 3 * Internal->Length[type] * (BitSample / 8));
  }
  if (BitSample == 8)
  {
    const unsigned int mult = Internal->BitSize[type] / 8;
    const unsigned int mult2 = length / Internal->Length[type];
    assert(Internal->Length[type] * mult2 == length);
    if (Internal->Length[type] * mult == length || Internal->Length[type] * mult + 1 == length)
    {
      assert(mult2 == 1 || mult2 == 2);
      unsigned int offset = 0;
      if (mult == 2)
      {
        offset = 1;
      }
      for (unsigned int i = 0; i < Internal->Length[type]; ++i)
      {
        assert(i * mult + offset < length);
        assert(3 * i + type < Internal->RGB.size());
        Internal->RGB[3 * i + type] = array[i * mult + offset];
      }
    }
    else
    {
      unsigned int offset = 0;
      assert(mult2 == 2);
      for (unsigned int i = 0; i < Internal->Length[type]; ++i)
      {
        assert(i * mult2 + offset < length);
        assert(3 * i + type < Internal->RGB.size());
        Internal->RGB[3 * i + type] = array[i * mult2 + offset];
      }
    }
  }
  else if (BitSample == 16)
  {
    assert(Internal->Length[type] * (BitSample / 8) == length);
    void *           p = static_cast<void*>(&Internal->RGB[0]);
    const void *     varray = static_cast<const void*>(array);
    uint16_t *       uchar16 = static_cast<uint16_t *>(p);
    const uint16_t * array16 = static_cast<const uint16_t *>(varray);
    for (unsigned int i = 0; i < Internal->Length[type]; ++i)
    {
      assert(2 * i < length);
      assert(2 * (3 * i + type) < Internal->RGB.size());
      uchar16[3 * i + type] = array16[i];
    }
  }
}

void
LookupTable::GetLUT(LookupTableType type, unsigned char * array, unsigned int & length) const
{
  if (BitSample == 8)
  {
    const unsigned int mult = Internal->BitSize[type] / 8;
    length = Internal->Length[type] * mult;
    unsigned int offset = 0;
    if (mult == 2)
    {
      offset = 1;
    }
    for (unsigned int i = 0; i < Internal->Length[type]; ++i)
    {
      assert(i * mult + offset < length);
      assert(3 * i + type < Internal->RGB.size());
      array[i * mult + offset] = Internal->RGB[3 * i + type];
    }
  }
  else if (BitSample == 16)
  {
    length = Internal->Length[type] * (BitSample / 8);
    void *     p = static_cast<void*>(&Internal->RGB[0]);
    void *     varray = static_cast<void*>(array);
    uint16_t * uchar16 = static_cast<uint16_t *>(p);
    uint16_t * array16 = static_cast<uint16_t *>(varray);
    for (unsigned int i = 0; i < Internal->Length[type]; ++i)
    {
      assert(2 * i < length);
      assert(2 * (3 * i + type) < Internal->RGB.size());
      array16[i] = uchar16[3 * i + type];
    }
  }
}

void
LookupTable::GetLUTDescriptor(LookupTableType  type,
                              unsigned short & length,
                              unsigned short & subscript,
                              unsigned short & bitsize) const
{
  assert(type >= RED && type <= BLUE);
  if (Internal->Length[type] == 65536)
  {
    length = 0;
  }
  else
  {
    length = static_cast<unsigned short>(Internal->Length[type]);
  }
  subscript = Internal->Subscript[type];
  bitsize = Internal->BitSize[type];
  assert(subscript == 0);
  assert(bitsize == 8 || bitsize == 16);
}

void
LookupTable::InitializeRedLUT(unsigned short length, unsigned short subscript, unsigned short bitsize)
{
  InitializeLUT(RED, length, subscript, bitsize);
}

void
LookupTable::InitializeGreenLUT(unsigned short length, unsigned short subscript, unsigned short bitsize)
{
  InitializeLUT(GREEN, length, subscript, bitsize);
}

void
LookupTable::InitializeBlueLUT(unsigned short length, unsigned short subscript, unsigned short bitsize)
{
  InitializeLUT(BLUE, length, subscript, bitsize);
}

void
LookupTable::SetRedLUT(const unsigned char * red, unsigned int length)
{
  SetLUT(RED, red, length);
}

void
LookupTable::SetGreenLUT(const unsigned char * green, unsigned int length)
{
  SetLUT(GREEN, green, length);
}

void
LookupTable::SetBlueLUT(const unsigned char * blue, unsigned int length)
{
  SetLUT(BLUE, blue, length);
}

namespace
{

typedef union
{
  uint8_t  rgb[4];
  uint32_t I;
} U8;

typedef union
{
  uint16_t rgb[4];
  uint64_t I;
} U16;

struct ltstr8
{
  bool
  operator()(U8 u1, U8 u2) const
  {
    return u1.I < u2.I;
  }
};

struct ltstr16
{
  bool
  operator()(U16 u1, U16 u2) const
  {
    return u1.I < u2.I;
  }
};

} // end namespace

inline void
printrgb(const unsigned char * rgb)
{
  std::cout << int(rgb[0]) << "," << int(rgb[1]) << "," << int(rgb[2]);
}

void
LookupTable::Decode(std::istream & is, std::ostream & os) const
{
  assert(Initialized());
  if (BitSample == 8)
  {
    unsigned char idx;
    unsigned char rgb[3];
    while (!is.eof())
    {
      is.read(reinterpret_cast<char *>(&idx), 1);
      if (is.eof() || !is.good())
        break;
      if (IncompleteLUT)
      {
        assert(idx < Internal->Length[RED]);
        assert(idx < Internal->Length[GREEN]);
        assert(idx < Internal->Length[BLUE]);
      }
      rgb[RED] = Internal->RGB[3 * idx + RED];
      rgb[GREEN] = Internal->RGB[3 * idx + GREEN];
      rgb[BLUE] = Internal->RGB[3 * idx + BLUE];
      os.write(reinterpret_cast<char *>(rgb), 3);
    }
  }
  else if (BitSample == 16)
  {
    const void * p = static_cast<void*>(&Internal->RGB[0]);
    const uint16_t * rgb16 = static_cast<const uint16_t *>(p);
    while (!is.eof())
    {
      unsigned short idx;
      unsigned short rgb[3];
      is.read(reinterpret_cast<char *>(&idx), 2);
      if (is.eof() || !is.good())
      {
        break;
      }
      if (IncompleteLUT)
      {
        assert(idx < Internal->Length[RED]);
        assert(idx < Internal->Length[GREEN]);
        assert(idx < Internal->Length[BLUE]);
      }
      rgb[RED] = rgb16[3 * idx + RED];
      rgb[GREEN] = rgb16[3 * idx + GREEN];
      rgb[BLUE] = rgb16[3 * idx + BLUE];
      os.write(reinterpret_cast<char *>(rgb), 3 * 2);
    }
  }
}

bool
LookupTable::Decode(char * output, size_t outlen, const char * input, size_t inlen) const
{
  bool success = false;
  if (outlen < 3 * inlen)
  {
    mdcmDebugMacro("Out buffer too small");
    return false;
  }
  if (!Initialized())
  {
    mdcmDebugMacro("Not Initialized");
    return false;
  }
  if (BitSample == 8)
  {
    const unsigned char * end = reinterpret_cast<const unsigned char *>(reinterpret_cast<uintptr_t>(input) + inlen);
    unsigned char *       rgb = reinterpret_cast<unsigned char *>(output);
    for (const unsigned char * idx = reinterpret_cast<const unsigned char *>(input); idx != end; ++idx)
    {
      if (IncompleteLUT)
      {
        assert(*idx < Internal->Length[RED]);
        assert(*idx < Internal->Length[GREEN]);
        assert(*idx < Internal->Length[BLUE]);
      }
      rgb[RED] = Internal->RGB[3 * *idx + RED];
      rgb[GREEN] = Internal->RGB[3 * *idx + GREEN];
      rgb[BLUE] = Internal->RGB[3 * *idx + BLUE];
      rgb += 3;
    }
    success = true;
  }
  else if (BitSample == 16)
  {
    const void * p = static_cast<void*>(&Internal->RGB[0]);
    const uint16_t * rgb16 = static_cast<const uint16_t *>(p);
    assert(inlen % 2 == 0);
    const void *     vinput = static_cast<const void*>(input);
    void *           voutput = static_cast<void*>(output);
    const uint16_t * end = reinterpret_cast<const uint16_t *>(reinterpret_cast<uintptr_t>(vinput) + inlen);
    uint16_t *       rgb = static_cast<uint16_t *>(voutput);
    for (const uint16_t * idx = static_cast<const uint16_t *>(vinput); idx != end; ++idx)
    {
      if (IncompleteLUT)
      {
        assert(*idx < Internal->Length[RED]);
        assert(*idx < Internal->Length[GREEN]);
        assert(*idx < Internal->Length[BLUE]);
      }
      rgb[RED] = rgb16[3 * *idx + RED];
      rgb[GREEN] = rgb16[3 * *idx + GREEN];
      rgb[BLUE] = rgb16[3 * *idx + BLUE];
      rgb += 3;
    }
    success = true;
  }
  return success;
}

int
LookupTable::DecodeSupplemental(char * output, size_t outlen, const char * input, size_t inlen) const
{
  if (outlen < 3 * inlen)
  {
    mdcmDebugMacro("Out buffer too small");
    return INT_MIN;
  }
  if (!Initialized())
  {
    mdcmDebugMacro("Not Initialized");
    return INT_MIN;
  }
  if (BitSample == 8)
  {
    const void *          vinput = static_cast<const void*>(input);
    void *                voutput = static_cast<void*>(output);
    const unsigned char * end = reinterpret_cast<const unsigned char *>(reinterpret_cast<uintptr_t>(vinput) + inlen);
    unsigned char *       rgb = static_cast<unsigned char *>(voutput);
    for (const unsigned char * idx = static_cast<const unsigned char *>(vinput); idx != end; ++idx)
    {
      if (static_cast<unsigned char>(*idx) >= Internal->Subscript[RED])
      {
        rgb[RED] = Internal->RGB[3 * ((*idx) - Internal->Subscript[RED]) + RED];
        rgb[GREEN] = Internal->RGB[3 * ((*idx) - Internal->Subscript[RED]) + GREEN];
        rgb[BLUE] = Internal->RGB[3 * ((*idx) - Internal->Subscript[RED]) + BLUE];
      }
      else
      {
        rgb[RED] = 0;
        rgb[GREEN] = 0;
        rgb[BLUE] = 0;
      }
      rgb += 3;
    }
    return static_cast<int>(Internal->Subscript[RED]);
  }
  else if (BitSample == 16)
  {
    const void *     p = static_cast<void *>(&Internal->RGB[0]);
    const void *     vinput = static_cast<const void*>(input);
    void *           voutput = static_cast<void*>(output);
    const uint16_t * rgb16 = static_cast<const uint16_t *>(p);
    assert(inlen % 2 == 0);
    const uint16_t * end = reinterpret_cast<const uint16_t *>(reinterpret_cast<uintptr_t>(vinput) + inlen);
    uint16_t *       rgb = static_cast<uint16_t *>(voutput);
    for (const uint16_t * idx = static_cast<const uint16_t *>(vinput); idx != end; ++idx)
    {
      if ((unsigned short)*idx >= Internal->Subscript[RED])
      {
        rgb[RED] = rgb16[3 * ((*idx) - Internal->Subscript[RED]) + RED];
        rgb[GREEN] = rgb16[3 * ((*idx) - Internal->Subscript[RED]) + GREEN];
        rgb[BLUE] = rgb16[3 * ((*idx) - Internal->Subscript[RED]) + BLUE];
      }
      else
      {
        rgb[RED] = 0;
        rgb[GREEN] = 0;
        rgb[BLUE] = 0;
      }
      rgb += 3;
    }
    return static_cast<int>(Internal->Subscript[RED]);
  }
  return INT_MIN;
}

const unsigned char *
LookupTable::GetPointer() const
{
  if (BitSample == 8)
  {
    return &Internal->RGB[0];
  }
  return NULL;
}

bool
LookupTable::GetBufferAsRGBA(unsigned char * rgba) const
{
  bool ret = false;
  if (BitSample == 8)
  {
    std::vector<unsigned char>::const_iterator it = Internal->RGB.cbegin();
    for (; it != Internal->RGB.cend();)
    {
      // RED
      *rgba++ = *it++;
      // GREEN
      *rgba++ = *it++;
      // BLUE
      *rgba++ = *it++;
      // ALPHA
      *rgba++ = 255;
    }
    ret = true;
  }
  else if (BitSample == 16)
  {
    void *     p = static_cast<void*>(&Internal->RGB[0]);
    void *     vrgba = static_cast<void*>(rgba);
    uint16_t * uchar16 = static_cast<uint16_t *>(p);
    uint16_t * rgba16 = static_cast<uint16_t *>(vrgba);
    size_t     s = Internal->RGB.size();
    s /= 2;
    s /= 3;
    memset(rgba, 0, Internal->RGB.size() * 4 / 3); // FIXME
    for (size_t i = 0; i < s; ++i)
    {
      // RED
      *rgba16++ = *uchar16++;
      // GREEN
      *rgba16++ = *uchar16++;
      // BLUE
      *rgba16++ = *uchar16++;
      // ALPHA
      *rgba16++ = 255 * 255;
    }
    ret = true;
  }
  return ret;
}

bool
LookupTable::WriteBufferAsRGBA(const unsigned char * rgba)
{
  bool ret = false;
  if (BitSample == 8)
  {
    std::vector<unsigned char>::iterator it = Internal->RGB.begin();
    for (; it != Internal->RGB.end();)
    {
      // RED
      *it++ = *rgba++;
      // GREEN
      *it++ = *rgba++;
      // BLUE
      *it++ = *rgba++;
      // ALPHA
      rgba++;
    }
    ret = true;
  }
  else if (BitSample == 16)
  {
    void *           p = static_cast<void*>(&Internal->RGB[0]);
    const void *     vrgba = static_cast<const void*>(rgba);
    uint16_t *       uchar16 = static_cast<uint16_t *>(p);
    const uint16_t * rgba16 = static_cast<const uint16_t *>(vrgba);
    size_t           s = Internal->RGB.size();
    s /= 2;
    s /= 3;
    assert(s == 65536);
    for (unsigned int i = 0; i < s; ++i)
    {
      // RED
      *uchar16++ = *rgba16++;
      // GREEN
      *uchar16++ = *rgba16++;
      // BLUE
      *uchar16++ = *rgba16++;
      //
      rgba16++;
    }
    ret = true;
  }
  return ret;
}

unsigned short
LookupTable::GetBitSample() const
{
  return BitSample;
}

void
LookupTable::Print(std::ostream &) const
{}

} // end namespace mdcm
