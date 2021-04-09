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
#include <stdexcept>

namespace mdcm
{

Preamble::Preamble()
{
  Create();
}

Preamble::~Preamble()
{
  if(Internal) delete[] Internal;
}

std::istream & Preamble::Read(std::istream & is)
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
        return is;
      }
    }
  }
  if(Internal)
  {
    delete[] Internal;
    Internal = NULL;
  }
  throw std::logic_error("Not a DICOM V3 file (No Preamble)");
}

void Preamble::Create()
{
  Internal = new char[128+4];
  memset(Internal, 0, 128);
  memcpy(Internal+128, "DICM", 4);
}

void Preamble::Remove()
{
  if(Internal)
  {
    delete[] Internal;
    Internal = NULL;
  }
}

std::ostream const & Preamble::Write(std::ostream & os) const
{
  if(Internal)
  {
    os.write(Internal, 128+4);
  }
  return os;
}

} // end namespace mdcm
