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

#include "mdcmVersion.h"

namespace mdcm
{

void Version::Print(std::ostream & os) const
{
  os << Version::GetVersion();
}

const char * Version::GetVersion()
{
  return MDCM_VERSION;
}

int Version::GetMajorVersion()
{
  return MDCM_MAJOR_VERSION;
}

int Version::GetMinorVersion()
{
  return MDCM_MINOR_VERSION;
}

int Version::GetBuildVersion()
{
  return MDCM_BUILD_VERSION;
}

}
