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
  See Copyright.txt or http://mdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef MDCMUNPACKER12BITS_H
#define MDCMUNPACKER12BITS_H

#include "mdcmTypes.h"

namespace mdcm
{
/*
 * Pack/Unpack 12 bits pixel into 16bits
 * Only pack an even number of 16 bits, which means a multiple of 4 (expressed in bytes)
 * Only unpack a multiple of 3 bytes
 *
 */
class MDCM_EXPORT Unpacker12Bits
{
public:
  // Pack an array of 16 bits where all values are 12 bits into a pack form.
  // n is the length in bytes of array in, out will be a fake 8 bits array of size
  // (n / 2) * 3
  static bool Pack(char *, const char *, size_t);

  // Unpack an array of 'packed' 12 bits data into a more conventional 16bits
  // array. n is the length in bytes of array in, out will be a 16 bits array of
  // size (n / 3) * 2
  static bool Unpack(char *, const char *, size_t);
};

}

#endif //MDCMUNPACKER12BITS_H
