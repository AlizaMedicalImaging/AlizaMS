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
#ifndef MDCMSWAPCODE_H
#define MDCMSWAPCODE_H

#include "mdcmTypes.h"
#include <iostream>

namespace mdcm
{

class MDCM_EXPORT SwapCode
{
public:
  typedef enum
  {
    Unknown = 0,
    LittleEndian = 1234,
    BigEndian = 4321,
    BadLittleEndian = 3412,
    BadBigEndian = 2143
  } SwapCodeType;

  operator SwapCode::SwapCodeType() const { return SwapCodeValue; }
  SwapCode(SwapCodeType sc = Unknown)
    : SwapCodeValue(sc)
  {}
  static const char *
  GetSwapCodeString(SwapCode const & sc);
  friend std::ostream &
  operator<<(std::ostream & os, const SwapCode & sc);

protected:
  static int
  GetIndex(SwapCode const & sc);

private:
  SwapCodeType SwapCodeValue;
};

inline std::ostream &
operator<<(std::ostream & os, const SwapCode & sc)
{
  os << SwapCode::GetSwapCodeString(sc);
  return os;
}

} // end namespace mdcm

#endif // MDCMSWAPCODE_H
