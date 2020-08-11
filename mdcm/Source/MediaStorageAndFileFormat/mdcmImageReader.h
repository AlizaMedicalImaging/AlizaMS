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
 * \brief ImageReader
 * \note its role is to convert the DICOM DataSet into a Image
 * representation
 * Image is different from Pixmap has it has a position and a direction in
 * Space.
 *
 * \see Image
 */
class MDCM_EXPORT ImageReader : public PixmapReader
{
public:
  ImageReader();
  virtual ~ImageReader();
  virtual bool Read();
  const Image& GetImage() const;
  Image& GetImage();

protected:
  bool ReadImage(MediaStorage const & ms);
  bool ReadACRNEMAImage();
};

} // end namespace mdcm

#endif //MDCMIMAGEREADER_H
