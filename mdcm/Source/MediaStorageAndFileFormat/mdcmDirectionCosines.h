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

#ifndef MDCMDIRECTIONCOSINES_H
#define MDCMDIRECTIONCOSINES_H

#include "mdcmTypes.h"

namespace mdcm
{

/**
 * Class to handle DirectionCosines
 */

class MDCM_EXPORT DirectionCosines
{
public:
  DirectionCosines();
  DirectionCosines(const double[6]);
  ~DirectionCosines() = default;
  bool
  IsValid() const;
  void
  Cross(double[3]) const;
  double
  Dot() const;
  static double
  Dot(const double[3], const double[3]);
  static void
  Normalize(double[3]);
  void
  Normalize();
  bool
  SetFromString(const char *);
  double
  CrossDot(DirectionCosines const &) const;
  double
  ComputeDistAlongNormal(const double[3]) const;
  void
  Print(std::ostream &) const;
  operator const double *() const { return Values; }

private:
  double Values[6];
};

} // end namespace mdcm

#endif // MDCMDIRECTIONCOSINES_H
