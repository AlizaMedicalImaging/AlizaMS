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
#ifndef MDCMIMAGEREADER_H
#define MDCMIMAGEREADER_H

#include "mdcmPixmapReader.h"
#include "mdcmImage.h"

namespace mdcm
{

class MediaStorage;
/**
 * ImageReader
 *
 * Image is different from Pixmap has it has a position and a direction in
 * Space.
 *
 */
class MDCM_EXPORT ImageReader : public PixmapReader
{
public:
  ImageReader();
  ~ImageReader();
  bool
  Read() override;
  const Image &
  GetImage() const;
  Image &
  GetImage();

protected:
  bool
  ReadImage(const MediaStorage &) override;
  bool
  ReadACRNEMAImage() override;
};

} // end namespace mdcm

#endif // MDCMIMAGEREADER_H
