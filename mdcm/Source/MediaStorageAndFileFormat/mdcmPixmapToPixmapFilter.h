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
#ifndef MDCMPIXMAPTOPIXMAPFILTER_H
#define MDCMPIXMAPTOPIXMAPFILTER_H

#include "mdcmBitmapToBitmapFilter.h"

namespace mdcm
{

class Pixmap;
/**
 * PixmapToPixmapFilter class
 * Super class for all filter taking an image and producing an output image
 */
class MDCM_EXPORT PixmapToPixmapFilter : public BitmapToBitmapFilter
{
public:
  PixmapToPixmapFilter() = default;
  ~PixmapToPixmapFilter() = default;
  Pixmap &
  GetInput();
  const Pixmap &
  GetOutput() const;
  const Pixmap &
  GetOutputAsPixmap() const;
};

} // end namespace mdcm

#endif // MDCMPIXMAPTOPIXMAPFILTER_H
