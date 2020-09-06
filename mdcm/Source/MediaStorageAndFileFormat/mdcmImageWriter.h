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
 * \brief ImageWriter
 */
class MDCM_EXPORT ImageWriter : public PixmapWriter
{
public:
  ImageWriter();
  ~ImageWriter();

  /// Set/Get Image to be written
  /// It will overwrite anything Image infos found in DataSet
  /// (see parent class to see how to pass dataset)
  const Image& GetImage() const { return dynamic_cast<const Image&>(*PixelData); }
  Image& GetImage() { return dynamic_cast<Image&>(*PixelData); } // FIXME
  //void SetImage(Image const &img);

  /// Write
  bool Write(); // Execute()

  /// internal function used to compute a target MediaStorage the most appropriate
  /// User may want to call this function ahead of time (before Write)
  MediaStorage ComputeTargetMediaStorage();
protected:

private:
};

} // end namespace mdcm

#endif //MDCMIMAGEWRITER_H
