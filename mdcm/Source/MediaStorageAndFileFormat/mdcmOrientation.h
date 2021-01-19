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
  friend std::ostream& operator<<(std::ostream &, const Orientation &);

public:
  Orientation();
  ~Orientation();
  void Print(std::ostream &) const;
  typedef enum
  {
    UNKNOWN,
    AXIAL,
    CORONAL,
    SAGITTAL,
    OBLIQUE
  } OrientationType;
  static OrientationType GetType(const double[6]);
  static void SetObliquityThresholdCosineValue(double);
  static double GetObliquityThresholdCosineValue();
  static const char * GetLabel(OrientationType);

protected:
  static char GetMajorAxisFromPatientRelativeDirectionCosine(double, double, double);

private:
  static double ObliquityThresholdCosineValue;
};

inline std::ostream& operator<<(std::ostream & os, const Orientation & o)
{
  o.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMORIENTATION_H
