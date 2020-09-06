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

#include "mdcmPreamble.h"
#include <cstring>

namespace mdcm
{

Preamble::Preamble():Internal(0)
{
  Create();
}

Preamble::~Preamble()
{
  delete[] Internal;
}

std::istream &Preamble::Read(std::istream &is)
{
  mdcmAssertAlwaysMacro(!IsEmpty());
  if(is.read(Internal, 128+4))
  {
    if(Internal[128+0] == 'D' &&
       Internal[128+1] == 'I' &&
       Internal[128+2] == 'C' &&
       Internal[128+3] == 'M')
    {
      return is;
    }
  }
  delete[] Internal;
  Internal = 0;
#ifndef MDCM_DONT_THROW
  throw Exception("Not a DICOM V3 file (No Preamble)");
#endif
}

void Preamble::Valid()
{
  if(!Internal) Internal = new char[128+4];
  memset(Internal, 0, 128);
  memcpy(Internal+128, "DICM", 4);
}

void Preamble::Create()
{
  if(!Internal) Internal = new char[128+4];
  memset(Internal, 0, 128);
  memcpy(Internal+128, "DICM", 4);
}

void Preamble::Remove()
{
  delete[] Internal;
  Internal = 0;
}

std::ostream const & Preamble::Write(std::ostream & os) const
{
  if(Internal)
  {
    os.write(Internal, 128+4);
  }
  return os;
}

void Preamble::Clear()
{
}

void Preamble::Print(std::ostream &) const
{
}

} // end namespace mdcm
