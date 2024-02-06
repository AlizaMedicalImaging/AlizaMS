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

#ifndef MDCMVERSION_H
#define MDCMVERSION_H

#include "mdcmTypes.h"

namespace mdcm
{

/*
 * Version
 *
 * major/minor and build version
 *
 */

class MDCM_EXPORT Version
{
public:
  static const char *
  GetVersion();
  static int
  GetMajorVersion();
  static int
  GetMinorVersion();
  static int
  GetBuildVersion();
};

} // end namespace mdcm

#endif // MDCMVERSION_H
