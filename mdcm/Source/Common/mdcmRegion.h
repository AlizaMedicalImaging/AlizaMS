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
#ifndef MDCMREGION_H
#define MDCMREGION_H

#include "mdcmTypes.h"
#include <vector>
#include <iostream>

namespace mdcm
{

class BoxRegion;

class MDCM_EXPORT Region
{
public:
  Region();
  virtual ~Region();
  virtual void
  Print(std::ostream & os = std::cout) const;
  virtual bool
  Empty() const = 0;
  virtual bool
  IsValid() const = 0;
  virtual size_t
  Area() const = 0;
  virtual BoxRegion
  ComputeBoundingBox() = 0;
};

inline std::ostream &
operator<<(std::ostream & os, const Region & r)
{
  r.Print(os);
  return os;
}

} // end namespace mdcm

#endif // MDCMREGION_H
