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

Preamble::Preamble() : Internal(NULL)
{
  Create();
}

Preamble::Preamble(Preamble const &)
{
  Create();
}

Preamble::~Preamble()
{
  if(Internal) delete[] Internal;
}

bool Preamble::Read(std::istream & is)
{
  if(!IsEmpty())
  {
    if(is.read(Internal, 128+4))
    {
      if(Internal[128+0] == 'D' &&
         Internal[128+1] == 'I' &&
         Internal[128+2] == 'C' &&
         Internal[128+3] == 'M')
      {
        return true;
      }
    }
  }
  if(Internal)
  {
    delete[] Internal;
    Internal = NULL;
  }
  return false;
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
  Internal = NULL;
}

void Preamble::Write(std::ostream & os) const
{
  if(Internal)
  {
    os.write(Internal, 128+4);
  }
}

void Preamble::Print(std::ostream & os) const
{
  os << Internal;
}

const char * Preamble::GetInternal() const
{
  return Internal;
}

bool Preamble::IsEmpty() const
{
  return !Internal;
}

VL Preamble::GetLength() const
{
  return (128 + 4);
}

} // end namespace mdcm
