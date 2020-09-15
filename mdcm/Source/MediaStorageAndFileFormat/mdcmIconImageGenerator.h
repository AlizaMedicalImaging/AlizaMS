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
#ifndef MDCMICONIMAGEGENERATOR_H
#define MDCMICONIMAGEGENERATOR_H

#include "mdcmPixmap.h"
#include "mdcmIconImage.h"

namespace mdcm
{
class IconImageGeneratorInternals;
/**
 * IconImageGenerator
 * This filter will generate a valid Icon from the Pixel Data element (an
 * instance of Pixmap).
 * To generate a valid Icon, one is only allowed the following Photometric
 * Interpretation:
 * - MONOCHROME1
 * - MONOCHROME2
 * - PALETTE_COLOR
 *
 * The Pixel Bits Allocated is restricted to 8bits, therefore 16 bits image
 * needs to be rescaled. By default the filter will use the full scalar range
 * of 16bits image to rescale to unsigned 8bits.
 * This may not be ideal for some situation, in which case the API
 * SetPixelMinMax can be used to overwrite the default min,max interval used.
 *
 */
class MDCM_EXPORT IconImageGenerator
{
public:
  IconImageGenerator();
  ~IconImageGenerator();
  void SetPixmap(const Pixmap & p) { P = p; }
  Pixmap & GetPixmap() { return *P; }
  const Pixmap & GetPixmap() const { return *P; }
  void SetOutputDimensions(const unsigned int dims[2]);
  void SetPixelMinMax(double min, double max);
  void AutoPixelMinMax(bool b);
  // Converting from RGB to PALETTE_COLOR can be a slow operation. However DICOM
  // standard requires that color icon be described as palette. Set this boolean
  // to false only if you understand the consequences.
  // default value is true, false generates invalid Icon Image Sequence
  void ConvertRGBToPaletteColor(bool b);
  // Requires AutoPixelMinMax(true)
  void SetOutsideValuePixel(double v);
  bool Generate();
  const IconImage& GetIconImage() const { return *I; }

protected:

private:
  void BuildLUT(Bitmap & bitmap, unsigned int maxcolor);
  SmartPointer<Pixmap> P;
  SmartPointer<IconImage> I;
  IconImageGeneratorInternals * Internals;
};

} // end namespace mdcm

#endif //MDCMICONIMAGEGENERATOR_H
