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

#ifndef MDCMPIXELFORMAT_H
#define MDCMPIXELFORMAT_H

#include "mdcmTypes.h"
#include <iostream>
#include <assert.h>

namespace mdcm
{

class TransferSyntax;

class MDCM_EXPORT PixelFormat
{
  friend class Bitmap;
  friend std::ostream& operator<<(std::ostream &_os, const PixelFormat &pf);

public:
  typedef enum
  {
    UINT8,
    INT8,
    UINT12,
    INT12,
    UINT16,
    INT16,
    UINT32,
    INT32,
    UINT64,
    INT64,
    FLOAT16,
    FLOAT32,
    FLOAT64,
    SINGLEBIT,
    UNKNOWN
  } ScalarType;

  explicit PixelFormat(
    unsigned short samplesperpixel = 1,
    unsigned short bitsallocated = 8,
    unsigned short bitsstored = 8,
    unsigned short highbit = 7,
    unsigned short pixelrepresentation = 0)
    :
    SamplesPerPixel(samplesperpixel),
    BitsAllocated(bitsallocated),
    BitsStored(bitsstored),
    HighBit(highbit),
    PixelRepresentation(pixelrepresentation) {}

  PixelFormat(ScalarType st);

  operator ScalarType() const { return GetScalarType(); }

  // Samples Per Pixel (0028,0002) US Samples Per Pixel
  unsigned short GetSamplesPerPixel() const;
  void SetSamplesPerPixel(unsigned short spp)
  {
    mdcmAssertMacro(spp <= 4);
    SamplesPerPixel = spp;
    assert(SamplesPerPixel == 1 || SamplesPerPixel == 3 || SamplesPerPixel == 4);
  }

  unsigned short GetBitsAllocated() const
  {
    return BitsAllocated;
  }
  void SetBitsAllocated(unsigned short ba)
  {
    if(ba > 0)
    {
      switch(ba)
      {
        // Some devices (FUJIFILM CR + MONO1) incorrectly set BitsAllocated/BitsStored
        // as bitmask instead of value. Do what they mean instead of what they say.
        case 0xffff: ba = 16; break;
        case 0x0fff: ba = 12; break;
        case 0x00ff: ba =  8; break;
      }
      BitsAllocated = ba;
      BitsStored = ba;
      HighBit = (unsigned short)(ba - 1);
    }
    else
    {
      BitsAllocated = 0;
      PixelRepresentation = 0;
    }
  }

  // BitsStored (0028,0101) US Bits Stored
  unsigned short GetBitsStored() const
  {
    assert(BitsStored <= BitsAllocated);
    return BitsStored;
  }
  void SetBitsStored(unsigned short bs)
  {
    switch(bs)
    {
      case 0xffff: bs = 16; break;
      case 0x0fff: bs = 12; break;
      case 0x00ff: bs =  8; break;
    }
    if(bs <= BitsAllocated && bs)
    {
      BitsStored = bs;
      SetHighBit((unsigned short) (bs - 1));
    }
  }

  // HighBit (0028,0102) US High Bit
  unsigned short GetHighBit() const
  {
    assert(HighBit < BitsStored);
    return HighBit;
  }
  void SetHighBit(unsigned short hb)
  {
    switch(hb)
    {
      case 0xfffe: hb = 15; break;
      case 0x0ffe: hb = 11; break;
      case 0x00fe: hb =  7; break;
    }
    if(hb < BitsStored) HighBit = hb;
  }

  // PixelRepresentation: 0 or 1, (0028,0103) US Pixel Representation
  unsigned short GetPixelRepresentation() const
  {
    return (unsigned short)(PixelRepresentation ? 1 : 0);
  }
  void SetPixelRepresentation(unsigned short pr)
  {
    PixelRepresentation = (unsigned short)(pr ? 1 : 0);
  }

  // ScalarType does not take into account the sample per pixel
  ScalarType GetScalarType() const;

  // Set PixelFormat based only on the ScalarType
  // Need to call SetScalarType *before* SetSamplesPerPixel
  void SetScalarType(ScalarType st);
  const char *GetScalarTypeAsString() const;

  // This is the number of words it would take to store one pixel.
  // The return value takes into account the SamplesPerPixel.
  // In the rare case when BitsAllocated == 12, the function
  // assume word padding and value returned will be identical as if BitsAllocated == 16
  uint8_t GetPixelSize() const;

  void Print(std::ostream &os) const;

  double GetMin() const;

  double GetMax() const;

  bool IsValid() const;

  bool operator==(ScalarType st) const
  {
    return GetScalarType() == st;
  }
  bool operator!=(ScalarType st) const
  {
    return GetScalarType() != st;
  }
  bool operator==(const PixelFormat &pf) const
  {
    return
      SamplesPerPixel     == pf.SamplesPerPixel &&
      BitsAllocated       == pf.BitsAllocated &&
      BitsStored          == pf.BitsStored &&
      HighBit             == pf.HighBit &&
      PixelRepresentation == pf.PixelRepresentation;
  }
  bool operator!=(const PixelFormat &pf) const
  {
    return
      SamplesPerPixel     != pf.SamplesPerPixel ||
      BitsAllocated       != pf.BitsAllocated ||
      BitsStored          != pf.BitsStored ||
      HighBit             != pf.HighBit ||
      PixelRepresentation != pf.PixelRepresentation;
  }
  bool IsCompatible(const TransferSyntax & ts) const;

protected:
  bool Validate();

private:
  unsigned short SamplesPerPixel;
  unsigned short BitsAllocated;
  unsigned short BitsStored;
  unsigned short HighBit;
  unsigned short PixelRepresentation;
};

inline std::ostream& operator<<(std::ostream & os, const PixelFormat & pf)
{
  pf.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMPIXELFORMAT_H
