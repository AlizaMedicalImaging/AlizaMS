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
#ifndef MDCMFILENAME_H
#define MDCMFILENAME_H

#include "mdcmTypes.h"
#include <string>

namespace mdcm
{
/**
 * Class to manipulate file name
 */
class MDCM_EXPORT Filename
{
public:
  Filename(const char * filename = "")
    : FileName(filename ? filename : ""), Path(), Conversion() {}
  const char * GetFileName() const;
  const char * GetPath();
  const char * GetName();
  const char * GetExtension();
  const char * ToUnixSlashes();
  const char * ToWindowsSlashes();
  std::string Join(const char *, const char *);
  bool IsEmpty() const;
  operator const char * () const { return GetFileName(); }
  bool EndWith(const char ending[]) const;
private:
  std::string FileName;
  std::string Path;
  std::string Conversion;
};

} // end namespace mdcm

#endif //MDCMFILENAME_H
