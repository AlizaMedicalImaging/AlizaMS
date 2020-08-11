/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMDIRECTORY_H
#define MDCMDIRECTORY_H

#include "mdcmTypes.h"
#include <string>
#include <vector>
#include <iostream>
#include <assert.h>

namespace mdcm
{
class MDCM_EXPORT Directory
{
  friend std::ostream& operator<<(std::ostream & _os, const Directory & d);
public :
  Directory()  {}
  ~Directory() {}
  typedef std::string FilenameType;
  typedef std::vector<FilenameType> FilenamesType;
  void Print(std::ostream & os = std::cout) const;
  FilenameType  const & GetToplevel()  const { return Toplevel; }
  FilenamesType const & GetFilenames() const
  {
    assert(!(Toplevel.empty()) && "Need to call Explore first");
    return Filenames;
  }
  FilenamesType const & GetDirectories() const { return Directories; }
  unsigned int Load(FilenameType const &name, bool recursive = false);

protected:
  unsigned int Explore(FilenameType const &, bool);

private :
  FilenamesType Filenames;
  FilenamesType Directories;
  FilenameType Toplevel;
};

inline std::ostream& operator<<(std::ostream & os, const Directory & d)
{
  d.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMDIRECTORY_H
