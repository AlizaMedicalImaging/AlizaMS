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
#ifndef MDCMPHOTOMETRICINTERPRETATION_H
#define MDCMPHOTOMETRICINTERPRETATION_H

#include "mdcmTypes.h"
#include <iostream>

namespace mdcm
{

class TransferSyntax;

/**
 * Class to represent an PhotometricInterpretation
 */
class MDCM_EXPORT PhotometricInterpretation
{
  friend std::ostream &
  operator<<(std::ostream &, const PhotometricInterpretation &);

public:
  typedef enum
  {
    UNKNOWN = 0,
    MONOCHROME1,
    MONOCHROME2,
    PALETTE_COLOR,
    RGB,
    HSV,
    ARGB,
    CMYK,
    YBR_FULL,
    YBR_FULL_422,
    YBR_PARTIAL_422, // retired
    YBR_PARTIAL_420,
    YBR_ICT,
    YBR_RCT,
    PI_END
  } PIType;

  PhotometricInterpretation() = default;

  PhotometricInterpretation(PIType pi)
    : PIField(pi)
  {}

  static const char * GetPIString(PIType);

  static PIType
  GetPIType(const char *);

  const char *
  GetString() const;

  unsigned short
  GetSamplesPerPixel() const;

  bool
  IsLossy() const;

  bool
  IsLossless() const;

  static bool IsRetired(PIType);

  operator PIType() const { return PIField; }
  
  PIType
  GetType() const
  {
    return PIField;
  }

private:
  PIType PIField{UNKNOWN};
};

inline std::ostream &
operator<<(std::ostream & os, const PhotometricInterpretation & val)
{
  const char * s = PhotometricInterpretation::GetPIString(val.PIField);
  os << (s ? s : "");
  return os;
}

} // end namespace mdcm

#endif // MDCMPHOTOMETRICINTERPRETATION_H
