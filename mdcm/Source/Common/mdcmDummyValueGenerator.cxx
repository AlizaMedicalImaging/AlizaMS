/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmDummyValueGenerator.h"
#include "mdcmTrace.h"
#include "mdcmSystem.h"
#include "mdcmSHA1.h"
#include "mdcmMD5.h"
#include <cstring>

namespace mdcm
{

const char * DummyValueGenerator::Generate(const char * input)
{
  static char digest[2*20+1] = {};
  bool b = false;
  if(input)
  {
    b = MD5::Compute(input, strlen(input), digest);
    //b = SHA1::Compute(input, strlen(input), digest);
  }
  if(b) return digest;
  return 0;
}


} // end namespace mdcm
