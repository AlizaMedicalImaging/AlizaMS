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
#ifndef MDCMPREAMBLE_H
#define MDCMPREAMBLE_H

#include "mdcmTypes.h"
#include "mdcmVL.h"

namespace mdcm
{

/**
 * DICOM Preamble (Part 10)
 */

class MDCM_EXPORT Preamble
{
public:
  Preamble();
  ~Preamble();
  void Remove();
  bool Read(std::istream &);
  void Write(std::ostream &) const;
  void Print(std::ostream &) const;
  const char * GetInternal() const;
  bool IsEmpty() const;
  VL GetLength() const;

private:
  void Create();
  char * Internal;
};

} // end namespace mdcm

#endif //MDCMPREAMBLE_H
