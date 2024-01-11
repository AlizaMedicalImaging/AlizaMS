// This class must be completely re-written!

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
  ~ImageWriter() = default;
  const Image &
  GetImage() const;
  Image &
  GetImage();
  MediaStorage
  ComputeTargetMediaStorage();
  bool
  Write();
};

} // end namespace mdcm

#endif // MDCMIMAGEWRITER_H
