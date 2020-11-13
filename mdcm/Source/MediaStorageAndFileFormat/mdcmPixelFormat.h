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

namespace mdcm
{

class TransferSyntax;

class MDCM_EXPORT PixelFormat
{
  friend class Bitmap;
  friend std::ostream& operator<<(std::ostream &, const PixelFormat &);

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

  PixelFormat() : PixelFormat(1, 8, 8, 7, 0) {}

  explicit PixelFormat(
    unsigned short samplesperpixel,
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

  PixelFormat(ScalarType);

  operator ScalarType() const { return GetScalarType(); }

  // Samples Per Pixel (0028,0002) US Samples Per Pixel
  unsigned short GetSamplesPerPixel() const;

  void SetSamplesPerPixel(unsigned short);

  unsigned short GetBitsAllocated() const;

  void SetBitsAllocated(unsigned short);

  // BitsStored (0028,0101) US Bits Stored
  unsigned short GetBitsStored() const;

  void SetBitsStored(unsigned short);

  // HighBit (0028,0102) US High Bit
  unsigned short GetHighBit() const;

  void SetHighBit(unsigned short);

  // PixelRepresentation: 0 or 1, (0028,0103) US Pixel Representation
  unsigned short GetPixelRepresentation() const;

  void SetPixelRepresentation(unsigned short);

  // ScalarType does not take into account the sample per pixel
  ScalarType GetScalarType() const;

  // Set PixelFormat based only on the ScalarType
  // Need to call SetScalarType *before* SetSamplesPerPixel
  void SetScalarType(ScalarType st);

  const char * GetScalarTypeAsString() const;

  // This is the number of words it would take to store one pixel.
  // The return value takes into account the SamplesPerPixel.
  // In the rare case when BitsAllocated == 12, the function
  // assume word padding and value returned will be identical as if BitsAllocated == 16
  uint8_t GetPixelSize() const;

  void Print(std::ostream &) const;

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
