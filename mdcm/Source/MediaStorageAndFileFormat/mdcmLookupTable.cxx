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
#include "mdcmSwapper.h"
#include <set>
#include <climits>
#include <cstring>
#include <cstdint>
#include <map>
#include <algorithm>
#include <iterator>
#include <vector>
#include <deque>
#include <cmath>

// http://blog.goo.ne.jp/satomi_takeo/e/3643e5249b2a9650f9e10ef1c830e8b8

namespace
{

// abstract class for segment
template <typename EntryType>
class Segment
{
public:
  virtual ~Segment() {}
  typedef std::map<const EntryType *, const Segment *> SegmentMap;
  virtual bool
  Expand(const SegmentMap & instances, std::vector<EntryType> & expanded) const = 0;
  const EntryType *
  First() const
  {
    return first_;
  }
  const EntryType *
  Last() const
  {
    return last_;
  }
  struct ToMap
  {
    std::pair<typename SegmentMap::key_type, typename SegmentMap::mapped_type>
    operator()(const Segment * segment) const
    {
      return std::make_pair(segment->First(), segment);
    }
  };

protected:
  Segment(const EntryType * first, const EntryType * last)
  {
    first_ = first;
    last_ = last;
  }
  const EntryType * first_;
  const EntryType * last_;
};

// discrete segment (opcode = 0)
template <typename EntryType>
class DiscreteSegment : public Segment<EntryType>
{
public:
  typedef typename Segment<EntryType>::SegmentMap SegmentMap;
  explicit DiscreteSegment(const EntryType * first)
    : Segment<EntryType>(first, first + 2 + *(first + 1))
  {}
  virtual bool
  Expand(const SegmentMap &, std::vector<EntryType> & expanded) const
  {
    std::copy(this->first_ + 2, this->last_, std::back_inserter(expanded));
    return true;
  }
};

// linear segment (opcode = 1)
template <typename EntryType>
class LinearSegment : public Segment<EntryType>
{
public:
  typedef typename Segment<EntryType>::SegmentMap SegmentMap;
  explicit LinearSegment(const EntryType * first)
    : Segment<EntryType>(first, first + 3)
  {}
  virtual bool
  Expand(const SegmentMap &, std::vector<EntryType> & expanded) const
  {
    if (expanded.empty())
    {
      // linear segment can't be the first segment.
      return false;
    }
    const EntryType length = *(this->first_ + 1);
    const EntryType y0 = expanded.back();
    const EntryType y1 = *(this->first_ + 2);
    double    y01 = y1 - y0;
    for (EntryType i = 0; i < length; ++i)
    {
      const double v = round(static_cast<double>(y0) + (static_cast<double>(i) / static_cast<double>(length)) * y01);
      expanded.push_back(static_cast<EntryType>(v));
    }
    return true;
  }
};

// indirect segment (opcode = 2)
template <typename EntryType>
class IndirectSegment : public Segment<EntryType>
{
public:
  typedef typename Segment<EntryType>::SegmentMap SegmentMap;
  explicit IndirectSegment(const EntryType * first)
    : Segment<EntryType>(first, first + 2 + 4 / sizeof(EntryType))
  {}
  virtual bool
  Expand(const SegmentMap & instances, std::vector<EntryType> & expanded) const
  {
    if (instances.empty())
    {
      // some other segments are required as references.
      return false;
    }
    const EntryType *                   first_segment = instances.begin()->first;
    const unsigned short *              pOffset = reinterpret_cast<const unsigned short *>(this->first_ + 2);
    unsigned long                       offsetBytes = (*pOffset) | (static_cast<unsigned long>(*(pOffset + 1)) << 16);
    const EntryType *                   copied_part_head = first_segment + offsetBytes / sizeof(EntryType);
    typename SegmentMap::const_iterator ppHeadSeg = instances.find(copied_part_head);
    if (ppHeadSeg == instances.cend())
    {
      // referred segment not found
      return false;
    }
    EntryType                           nNumCopies = *(this->first_ + 1);
    typename SegmentMap::const_iterator ppSeg = ppHeadSeg;
    while (std::distance(ppHeadSeg, ppSeg) < nNumCopies)
    {
      assert(ppSeg != instances.cend());
      ppSeg->second->Expand(instances, expanded);
      ++ppSeg;
    }
    return true;
  }
};

template <typename EntryType>
void
ExpandPalette(const EntryType * raw_values, uint32_t length, std::vector<EntryType> & palette)
{
  typedef std::deque<Segment<EntryType> *> SegmentList;
  SegmentList                              segments;
  const EntryType *                        raw_seg = raw_values;
  while ((std::distance(raw_values, raw_seg) * sizeof(EntryType)) < length)
  {
    Segment<EntryType> * segment = nullptr;
    if (*raw_seg == 0)
    {
      segment = new DiscreteSegment<EntryType>(raw_seg);
    }
    else if (*raw_seg == 1)
    {
      segment = new LinearSegment<EntryType>(raw_seg);
    }
    else if (*raw_seg == 2)
    {
      segment = new IndirectSegment<EntryType>(raw_seg);
    }
    if (segment)
    {
      segments.push_back(segment);
      raw_seg = segment->Last();
    }
    else
    {
      // invalid opcode
      break;
    }
  }
  typename Segment<EntryType>::SegmentMap instances;
  std::transform(
    segments.begin(), segments.end(), std::inserter(instances, instances.end()), typename Segment<EntryType>::ToMap());
  typename SegmentList::iterator ppSeg = segments.begin();
  typename SegmentList::iterator endOfSegments = segments.end();
  for (; ppSeg != endOfSegments; ++ppSeg)
  {
    (*ppSeg)->Expand(instances, palette);
  }
  ppSeg = segments.begin();
  for (; ppSeg != endOfSegments; ++ppSeg)
  {
    delete *ppSeg;
  }
}

}

namespace mdcm
{

bool
LookupTable::Initialized() const
{
  const bool b1 = BitSample != 0;
  const bool b2 = Internal.BitSize[0] != 0 && Internal.BitSize[1] != 0 && Internal.BitSize[2] != 0;
  return b1 && b2;
}

void
LookupTable::Clear()
{
  BitSample = 0;
  IncompleteLUT = false;
  Internal.Length[0] = 0U;
  Internal.Length[1] = 0U;
  Internal.Length[2] = 0U;
  Internal.Subscript[0] = 0U;
  Internal.Subscript[1] = 0U;
  Internal.Subscript[2] = 0U;
  Internal.BitSize[0] = 0U;
  Internal.BitSize[1] = 0U;
  Internal.BitSize[2] = 0U;
  Internal.RGB.clear();
}

void
LookupTable::Allocate(unsigned short bitsample)
{
  if (bitsample == 8)
  {
    Internal.RGB.resize(256 * 3);
  }
  else if (bitsample == 16)
  {
    Internal.RGB.resize(65536 * 2 * 3);
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
  if (!(static_cast<int>(type) >= 0 && static_cast<int>(type) <= 2))
  {
    mdcmWarningMacro("Error in InitializeLUT,  LookupTableType " << type);
    return;
  }
  if (bitsize != 8 && bitsize != 16)
  {
    return;
  }
  if (length == 0)
  {
    Internal.Length[type] = 65536;
  }
  else
  {
    if (length != 256)
    {
      IncompleteLUT = true;
    }
    Internal.Length[type] = length;
  }
  Internal.Subscript[type] = subscript;
  Internal.BitSize[type] = bitsize;
}

unsigned int
LookupTable::GetLUTLength(LookupTableType type) const
{
  return Internal.Length[type];
}

void
LookupTable::SetLUT(LookupTableType enhanced_type, const unsigned char * array, unsigned int length)
{
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
  if (!(static_cast<int>(type) >= 0 && static_cast<int>(type) <= 2))
  {
    mdcmWarningMacro("Error in SetLUT (1),  LookupTableType " << enhanced_type);
    return;
  }
  if (Internal.Length[type] == 0)
  {
    mdcmWarningMacro("Error in SetLUT (2), Length[" << type << "] is 0");
    return;
  }
  if (!IncompleteLUT)
  {
    if (Internal.RGB.size() != 3 * Internal.Length[type] * (BitSample / 8))
    {
      mdcmWarningMacro("Error in SetLUT (3)");
      return;
    }
  }
  if (BitSample == 8)
  {
    const unsigned int mult = Internal.BitSize[type] / 8;
    const unsigned int mult2 = length / Internal.Length[type];
    if (Internal.Length[type] * mult2 != length)
    {
      mdcmWarningMacro("Error in SetLUT (4)");
      return;
    }
    if (Internal.Length[type] * mult == length || Internal.Length[type] * mult + 1 == length)
    {
      assert(mult2 == 1 || mult2 == 2);
      unsigned int offset = 0;
      if (mult == 2)
      {
        offset = 1;
      }
      for (unsigned int i = 0; i < Internal.Length[type]; ++i)
      {
        assert(i * mult + offset < length);
        assert(3 * i + type < Internal.RGB.size());
        Internal.RGB[3 * i + type] = array[i * mult + offset];
      }
    }
    else
    {
      const unsigned int offset = 0;
      assert(mult2 == 2);
      for (unsigned int i = 0; i < Internal.Length[type]; ++i)
      {
        assert(i * mult2 + offset < length);
        assert(3 * i + type < Internal.RGB.size());
        Internal.RGB[3 * i + type] = array[i * mult2 + offset];
      }
    }
  }
  else if (BitSample == 16)
  {
    if (Internal.Length[type] * (BitSample / 8) != length)
    {
      mdcmWarningMacro("Error in SetLUT (5)");
      return;
    }
    void *           p = static_cast<void*>(Internal.RGB.data());
    const void *     varray = static_cast<const void*>(array);
    uint16_t *       uchar16 = static_cast<uint16_t *>(p);
    const uint16_t * array16 = static_cast<const uint16_t *>(varray);
    for (unsigned int i = 0; i < Internal.Length[type]; ++i)
    {
      assert(2 * i < length);
      assert(2 * (3 * i + type) < Internal.RGB.size());
      uchar16[3 * i + type] = array16[i];
    }
  }
}

void
LookupTable::SetSegmentedLUT(LookupTableType enhanced_type, const unsigned char * array, unsigned int length)
{
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
  if (!(static_cast<int>(type) >= 0 && static_cast<int>(type) <= 2))
  {
    mdcmWarningMacro("Error in SetSegmentedLUT, LookupTableType " << enhanced_type);
    return;
  }
  if (Internal.Length[type] == 0)
  {
    mdcmWarningMacro("Error in SetSegmentedLUT, Length[" << type << "] is 0");
    return;
  }
  if (BitSample == 8)
  {
    unsigned char * copy = new unsigned char[length];
    for (size_t x = 0; x < length; ++x)
    {
      copy[x] = array[x];
    }
    void *               varray = static_cast<void*>(copy);
    uint8_t *            array8 = static_cast<uint8_t *>(varray);
    std::vector<uint8_t> palette;
    unsigned int         num_entries = GetLUTLength(type);
    palette.reserve(num_entries);
    ExpandPalette<uint8_t>(array8, length, palette);
    const void * vpalette = static_cast<const void*>(palette.data());
    SetLUT(enhanced_type, static_cast<const unsigned char *>(vpalette), static_cast<unsigned int>(palette.size()));
    delete [] copy;
  }
  else if (BitSample == 16)
  {
    assert(length % 2 == 0);
    unsigned char * copy = new unsigned char[length];
    for (size_t x = 0; x < length; ++x)
    {
      copy[x] = array[x];
    }
    void *                varray = static_cast<void*>(copy);
    uint16_t *            array16 = static_cast<uint16_t *>(varray);
    std::vector<uint16_t> palette;
    unsigned int          num_entries = GetLUTLength(type);
    palette.reserve(num_entries);
    SwapperNoOp::SwapArray(array16, length / 2);
    ExpandPalette<uint16_t>(array16, length, palette);
    const void * vpalette = static_cast<const void*>(palette.data());
    SetLUT(enhanced_type, static_cast<const unsigned char *>(vpalette), 2U * static_cast<unsigned int>(palette.size()));
    delete [] copy;
  }
}

void
LookupTable::GetLUT(LookupTableType type, unsigned char * array, unsigned int & length) const
{
  if (!(static_cast<int>(type) >= 0 && static_cast<int>(type) <= 2))
  {
    mdcmWarningMacro("Error in GetLUT, LookupTableType " << type);
    return;
  }
  if (BitSample == 8)
  {
    const unsigned int mult = Internal.BitSize[type] / 8;
    length = Internal.Length[type] * mult;
    unsigned int offset = 0;
    if (mult == 2)
    {
      offset = 1;
    }
    for (unsigned int i = 0; i < Internal.Length[type]; ++i)
    {
      assert(i * mult + offset < length);
      assert(3 * i + type < Internal.RGB.size());
      array[i * mult + offset] = Internal.RGB[3 * i + type];
    }
  }
  else if (BitSample == 16)
  {
    length = Internal.Length[type] * (BitSample / 8);
    const void *     p = static_cast<const void*>(Internal.RGB.data());
    void *           varray = static_cast<void*>(array);
    const uint16_t * uchar16 = static_cast<const uint16_t *>(p);
    uint16_t *       array16 = static_cast<uint16_t *>(varray);
    for (unsigned int i = 0; i < Internal.Length[type]; ++i)
    {
      assert(2 * i < length);
      assert(2 * (3 * i + type) < Internal.RGB.size());
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
  if (!(static_cast<int>(type) >= 0 && static_cast<int>(type) <= 2))
  {
    mdcmWarningMacro("Error in GetLUTDescriptor, LookupTableType " << type);
    return;
  }
  if (Internal.Length[type] == 65536)
  {
    length = 0;
  }
  else
  {
    length = static_cast<unsigned short>(Internal.Length[type]);
  }
  subscript = Internal.Subscript[type];
  bitsize = Internal.BitSize[type];
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
        if (idx >= Internal.Length[RED] || idx >= Internal.Length[GREEN] || idx >= Internal.Length[BLUE])
        {
          mdcmWarningMacro("Error in Decode (3)");
          return;
        }
      }
      rgb[RED] = Internal.RGB[3 * idx + RED];
      rgb[GREEN] = Internal.RGB[3 * idx + GREEN];
      rgb[BLUE] = Internal.RGB[3 * idx + BLUE];
      os.write(reinterpret_cast<char *>(rgb), 3);
    }
  }
  else if (BitSample == 16)
  {
    const void * p = static_cast<const void*>(Internal.RGB.data());
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
        if (idx >= Internal.Length[RED] || idx >= Internal.Length[GREEN] || idx >= Internal.Length[BLUE])
        {
          mdcmWarningMacro("Error in Decode (4)");
          return;
        }
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
        if (*idx >= Internal.Length[RED] || *idx >= Internal.Length[GREEN] || *idx >= Internal.Length[BLUE])
        {
          mdcmWarningMacro("Error in Decode (1)");
          return false;
        }
      }
      rgb[RED] = Internal.RGB[3 * (*idx) + RED];
      rgb[GREEN] = Internal.RGB[3 * (*idx) + GREEN];
      rgb[BLUE] = Internal.RGB[3 * (*idx) + BLUE];
      rgb += 3;
    }
  }
  else if (BitSample == 16)
  {
    const void * p = static_cast<const void*>(Internal.RGB.data());
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
        if (*idx >= Internal.Length[RED] || *idx >= Internal.Length[GREEN] || *idx >= Internal.Length[BLUE])
        {
          mdcmWarningMacro("Error in Decode (2)");
          return false;
        }
      }
      rgb[RED] = rgb16[3 * (*idx) + RED];
      rgb[GREEN] = rgb16[3 * (*idx) + GREEN];
      rgb[BLUE] = rgb16[3 * (*idx) + BLUE];
      rgb += 3;
    }
  }
  return true;
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
      if (static_cast<unsigned char>(*idx) >= Internal.Subscript[RED])
      {
        rgb[RED] = Internal.RGB[3 * ((*idx) - Internal.Subscript[RED]) + RED];
        rgb[GREEN] = Internal.RGB[3 * ((*idx) - Internal.Subscript[RED]) + GREEN];
        rgb[BLUE] = Internal.RGB[3 * ((*idx) - Internal.Subscript[RED]) + BLUE];
      }
      else
      {
        rgb[RED] = 0;
        rgb[GREEN] = 0;
        rgb[BLUE] = 0;
      }
      rgb += 3;
    }
    return static_cast<int>(Internal.Subscript[RED]);
  }
  else if (BitSample == 16)
  {
    const void *     p = static_cast<const void *>(Internal.RGB.data());
    const void *     vinput = static_cast<const void*>(input);
    void *           voutput = static_cast<void*>(output);
    const uint16_t * rgb16 = static_cast<const uint16_t *>(p);
    assert(inlen % 2 == 0);
    const uint16_t * end = reinterpret_cast<const uint16_t *>(reinterpret_cast<uintptr_t>(vinput) + inlen);
    uint16_t *       rgb = static_cast<uint16_t *>(voutput);
    for (const uint16_t * idx = static_cast<const uint16_t *>(vinput); idx != end; ++idx)
    {
      if (static_cast<unsigned short>(*idx) >= Internal.Subscript[RED])
      {
        rgb[RED] = rgb16[3 * ((*idx) - Internal.Subscript[RED]) + RED];
        rgb[GREEN] = rgb16[3 * ((*idx) - Internal.Subscript[RED]) + GREEN];
        rgb[BLUE] = rgb16[3 * ((*idx) - Internal.Subscript[RED]) + BLUE];
      }
      else
      {
        rgb[RED] = 0;
        rgb[GREEN] = 0;
        rgb[BLUE] = 0;
      }
      rgb += 3;
    }
    return static_cast<int>(Internal.Subscript[RED]);
  }
  return INT_MIN;
}

const unsigned char *
LookupTable::GetPointer() const
{
  if (BitSample == 8)
  {
    return Internal.RGB.data();
  }
  return nullptr;
}

#if 0
bool
LookupTable::GetBufferAsRGBA(unsigned char * rgba) const
{
  bool ret = false;
  if (BitSample == 8)
  {
    std::vector<unsigned char>::const_iterator it = Internal.RGB.cbegin();
    for (; it != Internal.RGB.cend();)
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
    const void *     p = static_cast<const void*>(Internal.RGB.data());
    void *           vrgba = static_cast<void*>(rgba);
    const uint16_t * uchar16 = static_cast<const uint16_t *>(p);
    uint16_t *       rgba16 = static_cast<uint16_t *>(vrgba);
    size_t           s = Internal.RGB.size();
    s /= 2;
    s /= 3;
    memset(rgba, 0, Internal.RGB.size() * 4 / 3); // FIXME
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
    std::vector<unsigned char>::iterator it = Internal.RGB.begin();
    for (; it != Internal.RGB.end();)
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
    void *           p = static_cast<void*>(Internal.RGB.data());
    const void *     vrgba = static_cast<const void*>(rgba);
    uint16_t *       uchar16 = static_cast<uint16_t *>(p);
    const uint16_t * rgba16 = static_cast<const uint16_t *>(vrgba);
    size_t           s = Internal.RGB.size();
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
#endif

unsigned short
LookupTable::GetBitSample() const
{
  return BitSample;
}

} // end namespace mdcm
