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
#ifndef MDCMIMAGE_H
#define MDCMIMAGE_H

#include "mdcmPixmap.h"

#include <vector>

namespace mdcm
{

/**
 * Image
 * This is the container for an Image in the general sense.
 * From this container you should be able to request information like:
 * - Origin
 * - Dimension
 * - PixelFormat
 * But also to retrieve the image as a raw buffer (char *)
 * Since we have to deal with both RAW data and JPEG stream (which
 * internally encode all the above information) this API might seems
 * redundant. One way to solve that would be to subclass Image
 * with JPEGImage which would from the stream extract the header info
 * and fill it to please Image...well except origin for instance
 *
 * Basically you can see it as a storage for the Pixel Data element (7fe0,0010).
 *
 * This class does some heuristics to guess the Spacing but is not
 * compatible with DICOM CP-586. In case of doubt use PixmapReader instead
 *
 */
class MDCM_EXPORT Image : public Pixmap
{
public:
  Image();
  ~Image();
  const double * GetSpacing() const;
  double GetSpacing(unsigned int) const;
  void SetSpacing(const double *);
  void SetSpacing(unsigned int, double);
  const double * GetOrigin() const;
  double GetOrigin(unsigned int) const;
  void SetOrigin(const float *);
  void SetOrigin(const double *);
  void SetOrigin(unsigned int, double);
  const double * GetDirectionCosines() const;
  double GetDirectionCosines(unsigned int) const;
  void SetDirectionCosines(const float *);
  void SetDirectionCosines(const double *);
  void SetDirectionCosines(unsigned int, double);
  void SetIntercept(double);
  double GetIntercept() const;
  void SetSlope(double);
  double GetSlope() const;
  void SetWindowWidth(const std::string&);
  void SetWindowCenter(const std::string&);
  void SetWindowFunction(const std::string&);
  std::string GetWindowWidth() const;
  std::string GetWindowCenter() const;
  std::string GetWindowFunction() const;
  void Print(std::ostream &) const override;
private:
  SwapCode SC;
  double Intercept;
  double Slope;
  std::vector<double> Spacing;
  std::vector<double> Origin;
  std::vector<double> DirectionCosines;
  std::string WindowWidth;
  std::string WindowCenter;
  std::string WindowFunction;
};

} // end namespace mdcm

#endif //MDCMIMAGE_H
