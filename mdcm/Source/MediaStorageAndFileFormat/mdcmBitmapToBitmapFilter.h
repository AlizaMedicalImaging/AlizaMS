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
#ifndef MDCMBITMAPTOBITMAPFILTER_H
#define MDCMBITMAPTOBITMAPFILTER_H

#include "mdcmBitmap.h"

namespace mdcm
{

/**
 * BitmapToBitmapFilter class
 * Super class for all filter taking an image and producing an output image
 */
class MDCM_EXPORT BitmapToBitmapFilter
{
public:
  BitmapToBitmapFilter();
  ~BitmapToBitmapFilter() {}
  void
  SetInput(const Bitmap &);
  const Bitmap &
  GetOutput() const;
  const Bitmap &
  GetOutputAsBitmap() const; // SWIG/Java hack
protected:
  SmartPointer<Bitmap> Input;
  SmartPointer<Bitmap> Output;
};

} // end namespace mdcm

#endif // MDCMBITMAPTOBITMAPFILTER_H
