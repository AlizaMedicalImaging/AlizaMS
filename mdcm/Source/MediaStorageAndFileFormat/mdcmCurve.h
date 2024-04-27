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
#ifndef MDCMCURVE_H
#define MDCMCURVE_H

#include "mdcmTypes.h"
#include "mdcmObject.h"
#include <vector>

namespace mdcm
{

class DataSet;
class DataElement;

class CurveInternal
{
public:
  unsigned short              Group{};
  unsigned short              Dimensions{};
  unsigned short              NumberOfPoints{};
  std::string                 TypeOfData;
  std::string                 CurveDescription;
  unsigned short              DataValueRepresentation{};
  unsigned short              CoordinateStartValue{};
  unsigned short              CoordinateStepValue{};
  std::vector<char>           Data;
  std::vector<unsigned short> CurveDataDescriptor;
};

/**
 * Curve class to handle element 50xx,3000 Curve Data
 *  This is deprecated and last defined in PS 3.3 - 2004
 *
 */
class MDCM_EXPORT Curve : public Object
{
public:
  Curve() = default;
  Curve(const Curve & o) : Object(o)
  {
    Internal = o.Internal;
  }
  ~Curve() = default;
  static unsigned int
  GetNumberOfCurves(const DataSet &);
  void
  Update(const DataElement &);
  void
  SetGroup(unsigned short);
  unsigned short
  GetGroup() const;
  void
  SetDimensions(unsigned short);
  unsigned short
  GetDimensions() const;
  void
  SetNumberOfPoints(unsigned short);
  unsigned short
  GetNumberOfPoints() const;
  void
  SetTypeOfData(const char *);
  const char *
  GetTypeOfData() const;
  const char *
  GetTypeOfDataDescription() const;
  void
  SetCurveDescription(const char *);
  void
  SetDataValueRepresentation(unsigned short);
  unsigned short
  GetDataValueRepresentation() const;
  void
  SetCurveDataDescriptor(const uint16_t *, size_t);
  const std::vector<unsigned short> &
  GetCurveDataDescriptor() const;
  void
  SetCoordinateStartValue(unsigned short);
  void
  SetCoordinateStepValue(unsigned short);
  bool
  IsEmpty() const;
  void
  SetCurve(const char *, unsigned int);
  void
  Decode(std::istream &, std::ostream &);
  void
  GetAsPoints(float *) const;
  void
  Print(std::ostream &) const override;

private:
  double
  ComputeValueFromStartAndStep(unsigned int) const;
  CurveInternal Internal{};
};

} // end namespace mdcm

#endif // MDCMCURVE_H
