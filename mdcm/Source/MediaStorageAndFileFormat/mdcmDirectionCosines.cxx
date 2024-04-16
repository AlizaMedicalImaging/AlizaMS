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

#include "mdcmDirectionCosines.h"
#include <limits>
#include <cmath>
#include <cstdio>
#include <locale>

namespace mdcm
{

DirectionCosines::DirectionCosines(const double dircos[6])
{
  Values[0] = dircos[0];
  Values[1] = dircos[1];
  Values[2] = dircos[2];
  Values[3] = dircos[3];
  Values[4] = dircos[4];
  Values[5] = dircos[5];
}

bool
DirectionCosines::IsValid() const
{
  const double epsilon = 1e-3;
  const double norm_v1 = Values[0] * Values[0] + Values[1] * Values[1] + Values[2] * Values[2];
  const double norm_v2 = Values[3] * Values[3] + Values[4] * Values[4] + Values[5] * Values[5];
  if (fabs(norm_v1 - 1) < epsilon && fabs(norm_v2 - 1) < epsilon)
  {
    const double dot = Dot();
    if (fabs(dot) < epsilon)
    {
      return true;
    }
  }
  return false;
}

void
DirectionCosines::Cross(double z[3]) const
{
  z[0] = Values[1] * Values[5] - Values[2] * Values[4];
  z[1] = Values[2] * Values[3] - Values[0] * Values[5];
  z[2] = Values[0] * Values[4] - Values[1] * Values[3];
}

double
DirectionCosines::Dot(const double x[3], const double y[3])
{
  const double d = x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
  return d;
}

double
DirectionCosines::Dot() const
{
  const double d = Values[0] * Values[3] + Values[1] * Values[4] + Values[2] * Values[5];
  return d;
}

void
DirectionCosines::Normalize(double v[3])
{
  const double j = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (j != 0.0)
  {
    for (unsigned int i = 0; i < 3; ++i)
    {
      v[i] /= j;
    }
  }
}

void
DirectionCosines::Normalize()
{
  const double j = sqrt(Values[0] * Values[0] + Values[1] * Values[1] + Values[2] * Values[2]);
  if (j != 0.0)
  {
    for (unsigned int i = 0; i < 3; ++i)
    {
      Values[i] /= j;
    }
  }
  const double k = sqrt(Values[3] * Values[3] + Values[4] * Values[4] + Values[5] * Values[5]);
  if (k != 0.0)
  {
    for (unsigned int i = 3; i < 6; ++i)
    {
      Values[i] /= k;
    }
  }
}

bool
DirectionCosines::SetFromString(const char * s)
{
  if (s)
  {
    std::locale l = std::locale::global(std::locale::classic());
    const int   n =
      sscanf(s, "%lf\\%lf\\%lf\\%lf\\%lf\\%lf", Values, Values + 1, Values + 2, Values + 3, Values + 4, Values + 5);
    std::locale::global(l);
    if (n == 6)
    {
      return true;
    }
  }
  Values[0] = 1;
  Values[1] = 0;
  Values[2] = 0;
  Values[3] = 0;
  Values[4] = 1;
  Values[5] = 0;
  return false;
}

double
DirectionCosines::CrossDot(const DirectionCosines & dc) const
{
  double z1[3];
  Cross(z1);
  double z2[3];
  dc.Cross(z2);
  const double * x = z1;
  const double * y = z2;
  return x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
}

double
DirectionCosines::ComputeDistAlongNormal(const double ipp[3]) const
{
  double normal[3];
  Cross(normal);
  double dist = 0.;
  for (int i = 0; i < 3; ++i)
    dist += normal[i] * ipp[i];
  return dist;
}

void
DirectionCosines::Print(std::ostream & os) const
{
  os << Values[0] << ",";
  os << Values[1] << ",";
  os << Values[2] << ",";
  os << Values[3] << ",";
  os << Values[4] << ",";
  os << Values[5];
}

} // end namespace mdcm
