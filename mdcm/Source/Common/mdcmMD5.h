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
#ifndef MDCMMD5_H
#define MDCMMD5_H

#include "mdcmTypes.h"

namespace mdcm
{

class MDCM_EXPORT MD5
{
public:
  // Compute md5 from memory pointed by pointer
  static bool
  Compute(const char *, size_t, char[33]);
  // Compute md5 from a file
  static bool
  ComputeFile(const char *, char[33]);
};

} // end namespace mdcm

#endif // MDCMMD5_H
