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
#include "mdcmImage.h"
#include "mdcmTrace.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmByteValue.h"
#include "mdcmDataSet.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmFragment.h"
#include <iostream>

namespace mdcm
{

const double *
Image::GetSpacing() const
{
  return Spacing.data();
}

double
Image::GetSpacing(unsigned int idx) const
{
  return Spacing[idx];
}

void
Image::SetSpacing(const double spacing[3])
{
  Spacing[0] = spacing[0];
  Spacing[1] = spacing[1];
  if (NumberOfDimensions == 3)
    Spacing[2] = spacing[2];
  else
    Spacing[2] = 1.0;
}

void
Image::SetSpacing(unsigned int idx, double spacing)
{
  if (idx < 3)
    Spacing[idx] = spacing;
}

const double *
Image::GetOrigin() const
{
  return Origin.data();
}

double
Image::GetOrigin(unsigned int idx) const
{
  if (idx < 3)
    return Origin[idx];
  return 0.0;
}

void
Image::SetOrigin(const float ori[3])
{
  for (unsigned int i = 0; i < 3; ++i)
  {
    Origin[i] = ori[i];
  }
}

void
Image::SetOrigin(const double ori[3])
{
  for (unsigned int i = 0; i < 3; ++i)
  {
    Origin[i] = ori[i];
  }
}

void
Image::SetOrigin(unsigned int idx, double ori)
{
  if (idx < 3)
    Origin[idx] = ori;
}

const double *
Image::GetDirectionCosines() const
{
  return DirectionCosines.data();
}

double
Image::GetDirectionCosines(unsigned int idx) const
{
  if (idx < 6)
    return DirectionCosines[idx];
  return 0.0;
}

void
Image::SetDirectionCosines(const float dircos[6])
{
  for (unsigned int i = 0; i < 6; ++i)
  {
    DirectionCosines[i] = dircos[i];
  }
}

void
Image::SetDirectionCosines(const double dircos[6])
{
  for (unsigned int i = 0; i < 6; ++i)
  {
    DirectionCosines[i] = dircos[i];
  }
}

void
Image::SetDirectionCosines(unsigned int idx, double dircos)
{
  if (idx < 6)
    DirectionCosines[idx] = dircos;
}

void
Image::SetIntercept(double intercept)
{
  Intercept = intercept;
}

double
Image::GetIntercept() const
{
  return Intercept;
}

void
Image::SetSlope(double slope)
{
  Slope = slope;
}

double
Image::GetSlope() const
{
  return Slope;
}

void
Image::SetWindowWidth(const std::string & s)
{
  WindowWidth = s;
}

void
Image::SetWindowCenter(const std::string & s)
{
  WindowCenter = s;
}

void
Image::SetWindowFunction(const std::string & s)
{
  WindowFunction = s;
}

std::string
Image::GetWindowWidth() const
{
  return WindowWidth;
}

std::string
Image::GetWindowCenter() const
{
  return WindowCenter;
}

std::string
Image::GetWindowFunction() const
{
  return WindowFunction;
}

void
Image::Print(std::ostream & os) const
{
  Pixmap::Print(os);
  {
    os << "Origin: (";
    for (std::vector<double>::const_iterator it = Origin.cbegin(); it != Origin.cend(); ++it)
    {
      os << ',' << *it;
    }
    os << ")\n";
  }
  {
    os << "Spacing: (";
    for (std::vector<double>::const_iterator it = Spacing.cbegin(); it != Spacing.cend(); ++it)
    {
      os << ',' << *it;
    }
    os << ")\n";
  }
  {
    os << "DirectionCosines: (";
    for (std::vector<double>::const_iterator it = DirectionCosines.cbegin(); it != DirectionCosines.cend(); ++it)
    {
      os << ',' << *it;
    }
    os << ")\n";
  }
  os << "Rescale Intercept/Slope: (" << Intercept << ',' << Slope << ')' << std::endl;
}

} // end namespace mdcm
