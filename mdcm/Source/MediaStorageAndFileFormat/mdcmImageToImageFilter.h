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
#ifndef MDCMIMAGETOIMAGEFILTER_H
#define MDCMIMAGETOIMAGEFILTER_H

#include "mdcmPixmapToPixmapFilter.h"

namespace mdcm
{

class Image;
/**
 * ImageToImageFilter class
 * Super class for all filter taking an image and producing an output image
 */
class MDCM_EXPORT ImageToImageFilter : public PixmapToPixmapFilter
{
public:
  ImageToImageFilter();
  ~ImageToImageFilter() = default;
  Image &
  GetInput();
  const Image &
  GetOutput() const;
};

} // end namespace mdcm

#endif // MDCMIMAGETOIMAGEFILTER_H
