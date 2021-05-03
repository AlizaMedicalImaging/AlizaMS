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
#ifndef MDCMBYTESWAP_H
#define MDCMBYTESWAP_H

#include "mdcmTypes.h"
#include "mdcmSwapCode.h"

namespace mdcm
{

template <class T>
class ByteSwap
{
public:
  static bool
  SystemIsBigEndian();
  static bool
  SystemIsLittleEndian();
  static void
  Swap(T &);
  static void
  SwapFromSwapCodeIntoSystem(T &, SwapCode const &);
  static void
  SwapRange(T *, unsigned int);
  static void
  SwapRangeFromSwapCodeIntoSystem(T *, SwapCode const &, std::streamoff);
};

} // end namespace mdcm

#include "mdcmByteSwap.hxx"

#endif // MDCMBYTESWAP_H
