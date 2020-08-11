/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMIMAGECONVERTER_H
#define MDCMIMAGECONVERTER_H

#include "mdcmTypes.h"

namespace mdcm
{

class Image;
/**
 * \brief Image Converter
 * \note
 * This is the class used to convert from on Image to another
 * This filter is application level and not integrated directly in MDCM
 */
class MDCM_EXPORT ImageConverter
{
public:
  ImageConverter();
  ~ImageConverter();
  void SetInput(Image const & input);
  const Image & GetOuput() const;
  void Convert();

private:
  Image * Input;
  Image * Output;
};

} // end namespace mdcm

#endif //MDCMIMAGECONVERTER_H
