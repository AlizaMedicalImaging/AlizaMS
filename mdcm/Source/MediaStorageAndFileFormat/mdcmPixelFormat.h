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
  friend std::ostream &
  operator<<(std::ostream &, const PixelFormat &);

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
    FLOAT32,
    FLOAT64,
    SINGLEBIT,
    UNKNOWN
  } ScalarType;

  PixelFormat() = default;

  explicit PixelFormat(unsigned short samplesperpixel,
                       unsigned short bitsallocated,
                       unsigned short bitsstored,
                       unsigned short highbit,
                       unsigned short pixelrepresentation = 0)
    : SamplesPerPixel(samplesperpixel)
    , BitsAllocated(bitsallocated)
    , BitsStored(bitsstored)
    , HighBit(highbit)
    , PixelRepresentation(pixelrepresentation)
  {}

  PixelFormat(ScalarType);

  unsigned short
  GetSamplesPerPixel() const;

  void
  SetSamplesPerPixel(unsigned short);

  unsigned short
  GetBitsAllocated() const;

  void
  SetBitsAllocated(unsigned short);

  unsigned short
  GetBitsStored() const;

  void
  SetBitsStored(unsigned short);

  unsigned short
  GetHighBit() const;

  void
  SetHighBit(unsigned short);

  unsigned short
  GetPixelRepresentation() const;

  void
  SetPixelRepresentation(unsigned short);

  void
  SetScalarType(ScalarType st);

  ScalarType
  GetScalarType() const;

  const char *
  GetScalarTypeAsString() const;

  uint8_t
  GetPixelSize() const;

  double
  GetMin() const;

  double
  GetMax() const;

  bool
  IsValid() const;

  bool
  IsCompatible(const TransferSyntax &) const;

  void
  Print(std::ostream &) const;

  operator ScalarType() const { return GetScalarType(); }

  bool
  operator==(ScalarType st) const
  {
    return GetScalarType() == st;
  }

  bool
  operator!=(ScalarType st) const
  {
    return GetScalarType() != st;
  }

  bool
  operator==(const PixelFormat & pf) const
  {
    return SamplesPerPixel == pf.SamplesPerPixel && BitsAllocated == pf.BitsAllocated && BitsStored == pf.BitsStored &&
           HighBit == pf.HighBit && PixelRepresentation == pf.PixelRepresentation;
  }

  bool
  operator!=(const PixelFormat & pf) const
  {
    return SamplesPerPixel != pf.SamplesPerPixel || BitsAllocated != pf.BitsAllocated || BitsStored != pf.BitsStored ||
           HighBit != pf.HighBit || PixelRepresentation != pf.PixelRepresentation;
  }

protected:
  bool
  Validate();

private:
  unsigned short SamplesPerPixel{1};
  unsigned short BitsAllocated{8};
  unsigned short BitsStored{8};
  unsigned short HighBit{7};
  unsigned short PixelRepresentation{};
};

inline std::ostream &
operator<<(std::ostream & os, const PixelFormat & pf)
{
  pf.Print(os);
  return os;
}

} // end namespace mdcm

#endif // MDCMPIXELFORMAT_H
