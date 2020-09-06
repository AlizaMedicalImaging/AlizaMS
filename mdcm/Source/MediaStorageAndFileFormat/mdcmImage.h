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
 * \brief Image
 * \details This is the container for an Image in the general sense.
 * From this container you should be able to request information like:
 * - Origin
 * - Dimension
 * - PixelFormat
 * ...
 * But also to retrieve the image as a raw buffer (char *)
 * Since we have to deal with both RAW data and JPEG stream (which
 * internally encode all the above information) this API might seems
 * redundant. One way to solve that would be to subclass Image
 * with JPEGImage which would from the stream extract the header info
 * and fill it to please Image...well except origin for instance
 *
 * Basically you can see it as a storage for the Pixel Data element (7fe0,0010).
 *
 * \warning This class does some heuristics to guess the Spacing but is not
 * compatible with DICOM CP-586. In case of doubt use PixmapReader instead
 *
 * \see ImageReader PixmapReader
 */
class MDCM_EXPORT Image : public Pixmap
{
public:
  Image () : Spacing(),SC(),Intercept(0),Slope(1)
  {
    Origin.resize(3);
    DirectionCosines.resize(6);
    DirectionCosines[0] = 1;
    DirectionCosines[4] = 1;
    Spacing.resize(3, 1);
  }
  ~Image() {}
  /// Return a 3-tuples specifying the spacing
  /// NOTE: 3rd value can be an aribtrary 1 value when the spacing was not specified
  /// WARNING: when the spacing is not specifier, a default value of 1 will be returned
  const double * GetSpacing() const;
  double GetSpacing(unsigned int idx) const;
  void SetSpacing(const double  *spacing);
  void SetSpacing(unsigned int idx, double spacing);
  /// Return (0,0,0) if the origin was not specified.
  const double * GetOrigin() const;
  double GetOrigin(unsigned int idx) const;
  void SetOrigin(const float *ori);
  void SetOrigin(const double *ori);
  void SetOrigin(unsigned int idx, double ori);
  /// A default value of (1,0,0,0,1,0) will be return when the direction cosines was not specified.
  const double * GetDirectionCosines() const;
  double GetDirectionCosines(unsigned int idx) const;
  void SetDirectionCosines(const float *dircos);
  void SetDirectionCosines(const double *dircos);
  void SetDirectionCosines(unsigned int idx, double dircos);
  /// Print
  void Print(std::ostream &os) const;
  /// Intercept
  void SetIntercept(double intercept) { Intercept = intercept; }
  double GetIntercept() const { return Intercept; }
  /// Slope
  void SetSlope(double slope) { Slope = slope; }
  double GetSlope() const { return Slope; }

private:
  std::vector<double> Spacing;
  std::vector<double> Origin;
  std::vector<double> DirectionCosines;
  SwapCode SC;
  double Intercept;
  double Slope;
};

} // end namespace mdcm

#endif //MDCMIMAGE_H
