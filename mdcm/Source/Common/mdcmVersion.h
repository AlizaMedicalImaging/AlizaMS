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
#include <iostream>

namespace mdcm
{

/**
 * \class Version
 * \brief major/minor and build version
 */

class MDCM_EXPORT Version
{
  friend std::ostream& operator<<(std::ostream & _os, const Version & v);
public:
  static const char * GetVersion();
  static int GetMajorVersion();
  static int GetMinorVersion();
  static int GetBuildVersion();
  void Print(std::ostream &os = std::cout) const;
  Version() {};
  ~Version() {};
};

inline std::ostream& operator<<(std::ostream & os, const Version & v)
{
  v.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMVERSION_H
