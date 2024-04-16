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
  Internal = new char[132]{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    'D', 'I', 'C', 'M'};
}

Preamble::~Preamble()
{
  delete[] Internal;
}

std::istream &
Preamble::Read(std::istream & is)
{
  if (!IsEmpty())
  {
    if (is.read(Internal, 132))
    {
      if (Internal[128] == 'D' &&
          Internal[129] == 'I' &&
          Internal[130] == 'C' &&
          Internal[131] == 'M')
      {
        return is;
      }
    }
  }
  if (Internal)
  {
    delete[] Internal;
    Internal = nullptr;
  }
  throw std::logic_error("Not a DICOM V3 file (No Preamble)");
}

void
Preamble::Remove()
{
  if (Internal)
  {
    delete[] Internal;
    Internal = nullptr;
  }
}

const std::ostream &
Preamble::Write(std::ostream & os) const
{
  if (Internal)
  {
    os.write(Internal, 132);
  }
  return os;
}

} // end namespace mdcm
