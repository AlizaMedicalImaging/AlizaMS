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
 * \brief Class to represent an PhotometricInterpretation
 */
class MDCM_EXPORT PhotometricInterpretation
{
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

  PhotometricInterpretation(PIType pi = UNKNOWN) : PIField(pi) {}
  static const char * GetPIString(PIType pi);
  const char * GetString() const;
  //make sure end of string is \0
  static PIType GetPIType(const char * pi);
  static bool IsRetired(PIType pi);
  bool IsLossy() const;
  bool IsLossless() const;
  /// Return the value for Sample Per Pixel associated with a particular Photometric Interpretation
  unsigned short GetSamplesPerPixel() const;
  // TODO
  // not all PhotometricInterpretation are allowed for compressed transfer syntax
  friend std::ostream& operator<<(std::ostream & os, const PhotometricInterpretation & pi);
  operator PIType () const { return PIField; }
  PIType GetType () const { return PIField; }

private:
  PIType PIField;
};

inline std::ostream& operator<<(std::ostream & os, const PhotometricInterpretation & val)
{
  const char * s = PhotometricInterpretation::GetPIString(val.PIField);
  os << (s ? s : "");
  return os;
}

} // end namespace mdcm

#endif //MDCMPHOTOMETRICINTERPRETATION_H
