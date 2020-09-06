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
 * \brief DICOM Preamble (Part 10)
 */

class MDCM_EXPORT Preamble
{
public:
  Preamble();
  ~Preamble();
  friend std::ostream &operator<<(std::ostream &, const Preamble &);
  void Clear();
  void Valid();
  void Create();
  void Remove();
  std::istream & Read(std::istream &);
  std::ostream const & Write(std::ostream &) const;
  void Print(std::ostream &) const;
  const char * GetInternal() const { return Internal; }
  bool IsEmpty() const { return !Internal; }
  VL GetLength() const { return 128 + 4; }
  Preamble(Preamble const &)
  {
    Create();
  }
  Preamble& operator=(Preamble const &)
  {
    Create();
    return *this;
  }

protected:
  bool IsValid() const { return true; }

private:
  char * Internal;
};

inline std::ostream& operator<<(std::ostream & os, const Preamble & val)
{
  os << val.Internal;
  return os;
}

} // end namespace mdcm

#endif //MDCMPREAMBLE_H
