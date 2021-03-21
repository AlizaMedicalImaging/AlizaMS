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

Image::Image() : SC(), Intercept(0), Slope(1)
{
  Spacing.resize(3, 1);
  Origin.resize(3, 0);
  DirectionCosines.resize(6);
  DirectionCosines[0] = 1;
  DirectionCosines[1] = 0;
  DirectionCosines[2] = 0;
  DirectionCosines[3] = 0;
  DirectionCosines[4] = 1;
  DirectionCosines[5] = 0;
}

Image::~Image() {}

// Note: 3rd value can be an '1' when the spacing was not specified.
// Warning: when the spacing is not specifier, a default '1' will be returned.
const double * Image::GetSpacing() const
{
  assert(NumberOfDimensions);
  return &Spacing[0];
}

double Image::GetSpacing(unsigned int idx) const
{
  assert(NumberOfDimensions);
  return Spacing[idx];
}

void Image::SetSpacing(const double * spacing)
{
  assert(NumberOfDimensions);
  Spacing.assign(spacing, spacing+NumberOfDimensions);
}

void Image::SetSpacing(unsigned int idx, double spacing)
{
  Spacing.resize(3);
  Spacing[idx] = spacing;
}

// Return (0,0,0) if the origin was not specified.
const double * Image::GetOrigin() const
{
  assert(NumberOfDimensions);
  if(!Origin.empty())
    return &Origin[0];
  return 0;
}

double Image::GetOrigin(unsigned int idx) const
{
  assert(NumberOfDimensions);
  if(idx < Origin.size())
  {
    return Origin[idx];
  }
  return 0;
}

void Image::SetOrigin(const float * ori)
{
  assert(NumberOfDimensions);
  Origin.resize(NumberOfDimensions);
  for(unsigned int i = 0; i < NumberOfDimensions; ++i)
  {
    Origin[i] = static_cast<double>(ori[i]);
  }
}

void Image::SetOrigin(const double * ori)
{
  assert(NumberOfDimensions);
  Origin.assign(ori, ori+NumberOfDimensions);
}

void Image::SetOrigin(unsigned int idx, double ori)
{
  Origin.resize(idx + 1);
  Origin[idx] = ori;
}

// A default value of (1,0,0,0,1,0) will be return when the direction
// cosines was not specified.
const double * Image::GetDirectionCosines() const
{
  assert(NumberOfDimensions);
  if(!DirectionCosines.empty())
    return &DirectionCosines[0];
  return 0;
}
double Image::GetDirectionCosines(unsigned int idx) const
{
  assert(NumberOfDimensions);
  if(idx < DirectionCosines.size())
  {
    return DirectionCosines[idx];
  }
  return 0;
}

void Image::SetDirectionCosines(const float * dircos)
{
  assert(NumberOfDimensions);
  DirectionCosines.resize(6);
  for(int i = 0; i < 6; ++i)
  {
    DirectionCosines[i] = static_cast<double>(dircos[i]);
  }
}

void Image::SetDirectionCosines(const double * dircos)
{
  assert(NumberOfDimensions);
  DirectionCosines.assign(dircos, dircos+6);
}

void Image::SetDirectionCosines(unsigned int idx, double dircos)
{
  DirectionCosines.resize(idx + 1);
  DirectionCosines[idx] = dircos;
}

void Image::SetIntercept(double intercept)
{
  Intercept = intercept;
}

double Image::GetIntercept() const
{
  return Intercept;
}

void Image::SetSlope(double slope)
{
  Slope = slope;
}

double Image::GetSlope() const
{
  return Slope;
}

void Image::SetWindowWidth(const std::string & s)
{
  WindowWidth = s;
}

void Image::SetWindowCenter(const std::string & s)
{
  WindowCenter = s;
}

void Image::SetWindowFunction(const std::string & s)
{
  WindowFunction = s;
}

std::string Image::GetWindowWidth() const
{
  return WindowWidth;
}

std::string Image::GetWindowCenter() const
{
  return WindowCenter;
}

std::string Image::GetWindowFunction() const
{
  return WindowFunction;
}

void Image::Print(std::ostream & os) const
{
  Pixmap::Print(os);
  if(NumberOfDimensions)
  {
    {
      os << "Origin: (";
      if(!Origin.empty())
      {
        std::vector<double>::const_iterator it = Origin.begin();
        os << *it;
        for(++it; it != Origin.end(); ++it)
        {
          os << "," << *it;
        }
      }
      os << ")\n";
    }
    {
      os << "Spacing: (";
      std::vector<double>::const_iterator it = Spacing.begin();
      os << *it;
      for(++it; it != Spacing.end(); ++it)
      {
        os << "," << *it;
      }
      os << ")\n";
    }
    {
      os << "DirectionCosines: (";
      if(!DirectionCosines.empty())
      {
        std::vector<double>::const_iterator it = DirectionCosines.begin();
        os << *it;
        for(++it; it != DirectionCosines.end(); ++it)
        {
          os << "," << *it;
        }
      }
      os << ")\n";
    }
    {
      os << "Rescale Intercept/Slope: (" << Intercept << "," << Slope << ")\n";
    }
  }
}

} // end namespace mdcm
