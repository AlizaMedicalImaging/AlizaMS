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
#ifndef MDCMORIENTATION_H
#define MDCMORIENTATION_H

#include "mdcmTypes.h"

namespace mdcm
{

/**
 * Orientation
 */
class MDCM_EXPORT Orientation
{
public:
  typedef enum
  {
    UNKNOWN,
    AXIAL,
    CORONAL,
    SAGITTAL,
    OBLIQUE
  } OrientationType;

  static OrientationType
  GetType(const double[6]);

  static void
  SetObliquityThresholdCosineValue(double);

  static double
  GetObliquityThresholdCosineValue();
  
  static const char * GetLabel(OrientationType);

  static char
  GetMajorAxisFromPatientRelativeDirectionCosine(double, double, double);

private:
  static double ObliquityThresholdCosineValue;
};

} // end namespace mdcm

#endif // MDCMORIENTATION_H
