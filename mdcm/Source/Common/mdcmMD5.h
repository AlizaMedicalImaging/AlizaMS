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
public :
  // Compute md5 from memory pointed by `pointer` of size `buf_len`
  static bool Compute(const char * buffer, size_t buf_len, char digest_str[33]);

  // Compute md5 from a file `filename`
  static bool ComputeFile(const char * filename, char digest_str[33]);
};

} // end namespace mdcm

#endif //MDCMMD5_H
