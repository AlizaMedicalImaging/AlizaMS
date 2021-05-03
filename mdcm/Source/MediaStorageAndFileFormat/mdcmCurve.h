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

class CurveInternal;
class ByteValue;
class DataSet;
class DataElement;
/**
 * Curve class to handle element 50xx,3000 Curve Data
 *  WARNING: This is deprecated and lastly defined in PS 3.3 - 2004
 *
 *  Examples:
 *  - GE_DLX-8-MONO2-Multiframe-Jpeg_Lossless.dcm
 *  - GE_DLX-8-MONO2-Multiframe.dcm
 *  - mdcmSampleData/Philips_Medical_Images/integris_HV_5000/xa_integris.dcm
 *  - TOSHIBA-CurveData[1-3].dcm
 */
class MDCM_EXPORT Curve : public Object
{
public:
  Curve();
  Curve(Curve const &);
  ~Curve();
  static unsigned int
  GetNumberOfCurves(DataSet const &);
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
  std::vector<unsigned short> const &
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
  CurveInternal * Internal;
};

} // end namespace mdcm

#endif // MDCMCURVE_H
