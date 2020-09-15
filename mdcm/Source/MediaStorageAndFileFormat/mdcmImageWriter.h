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
#ifndef MDCMIMAGEWRITER_H
#define MDCMIMAGEWRITER_H

#include "mdcmPixmapWriter.h"
#include "mdcmImage.h"

namespace mdcm
{

class Image;
/**
 * ImageWriter
 */
class MDCM_EXPORT ImageWriter : public PixmapWriter
{
public:
  ImageWriter();
  ~ImageWriter();

  // It will overwrite anything Image infos found in DataSet
  // (see parent class to see how to pass dataset)
  const Image& GetImage() const { return dynamic_cast<const Image&>(*PixelData); }
  Image& GetImage() { return dynamic_cast<Image&>(*PixelData); } // FIXME
  bool Write();
  // Internal function used to compute a target MediaStorage
  // User may want to call this function ahead before Write
  MediaStorage ComputeTargetMediaStorage();
};

} // end namespace mdcm

#endif //MDCMIMAGEWRITER_H
