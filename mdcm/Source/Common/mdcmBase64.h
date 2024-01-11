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
#ifndef MDCMBASE64_H
#define MDCMBASE64_H

#include "mdcmTypes.h"
#include "mdcmMacro.h"

namespace mdcm
{

class MDCM_EXPORT Base64
{
public:
  MDCM_DISALLOW_COPY_AND_MOVE(Base64);
  /**
   * Call this function to obtain the required buffer size
   */
  static size_t
  GetEncodeLength(const char *, size_t);

  /**
   *
   * dst      destination buffer
   * dlen     size of the buffer
   * src      source buffer
   * slen     amount of data to be encoded
   *
   * return 0 if not successful, size of encoded otherwise
   *
   */
  static size_t
  Encode(char * dst, size_t dlen, const char * src, size_t slen);

  /**
   * Call this function to obtain the required buffer size
   */
  static size_t
  GetDecodeLength(const char *, size_t);

  /**
   *
   * dst      destination buffer
   * dlen     size of the buffer
   * src      source buffer
   * slen     amount of data to be decoded
   *
   * return 0 if not successful, size of decoded otherwise
   *
   */
  static size_t
  Decode(char * dst, size_t dlen, const char * src, size_t slen);
};

} // end namespace mdcm

#endif // MDCMBASE64_H
