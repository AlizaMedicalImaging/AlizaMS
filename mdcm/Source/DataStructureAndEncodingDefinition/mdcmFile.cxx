/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmFile.h"

namespace mdcm_ns
{

File::File() {}
File::~File() {}

std::istream & File::Read(std::istream & is)
{
  return is;
}

std::ostream const & File::Write(std::ostream & os) const
{
  return os;
}

} // end namespace mdcm_ns
