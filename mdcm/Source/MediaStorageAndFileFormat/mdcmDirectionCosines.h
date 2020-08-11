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
 * \brief class to handle DirectionCosines
 */

class MDCM_EXPORT DirectionCosines
{
public:
  DirectionCosines();
  DirectionCosines(const double dircos[6]);
  ~DirectionCosines();
  void Print(std::ostream &) const;
  void Cross(double z[3]) const;
  double Dot() const;
  static double Dot(const double x[3], const double y[3]);
  void Normalize();
  static void Normalize(double v[3]);
  operator const double* () const { return Values; }
  bool IsValid() const;
  bool SetFromString(const char * str);
  double CrossDot(DirectionCosines const & dc) const;
  double ComputeDistAlongNormal(const double ipp[3]) const;

private:
  double Values[6];
};

} // end namespace mdcm

#endif //MDCMDIRECTIONCOSINES_H
