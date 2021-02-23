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
#ifndef MDCMBOXREGION_H
#define MDCMBOXREGION_H

#include "mdcmRegion.h"

namespace mdcm
{

class BoxRegionInternals;

class MDCM_EXPORT BoxRegion : public Region
{
public :
  BoxRegion();
  ~BoxRegion();
  void SetDomain(
    unsigned int xmin, unsigned int xmax,
    unsigned int ymin, unsigned int ymax,
    unsigned int zmin, unsigned int zmax);
  unsigned int GetXMin() const;
  unsigned int GetXMax() const;
  unsigned int GetYMin() const;
  unsigned int GetYMax() const;
  unsigned int GetZMin() const;
  unsigned int GetZMax() const;
  bool Empty() const override;
  bool IsValid() const override;
  size_t Area() const override;
  BoxRegion ComputeBoundingBox() override;
  void Print(std::ostream & os = std::cout) const override;
  static BoxRegion BoundingBox(BoxRegion const &, BoxRegion const &);
  BoxRegion(const BoxRegion &);
  void operator=(const BoxRegion &);

private:
  BoxRegionInternals * Internals;
};

} // end namespace mdcm

#endif //MDCMREGION_H
