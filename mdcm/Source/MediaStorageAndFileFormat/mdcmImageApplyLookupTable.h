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
#ifndef MDCMIMAGEAPPLYLOOKUPTABLE_H
#define MDCMIMAGEAPPLYLOOKUPTABLE_H

#include "mdcmImageToImageFilter.h"

namespace mdcm
{

class DataElement;
/**
 * \brief ImageApplyLookupTable class
 * \details It applies the LUT the PixelData (only PALETTE_COLOR images)
 * Output will be a PhotometricInterpretation=RGB image
 */
class MDCM_EXPORT ImageApplyLookupTable : public ImageToImageFilter
{
public:
  ImageApplyLookupTable() {}
  ~ImageApplyLookupTable() {}
  bool Apply();

private:
};

} // end namespace mdcm

#endif //MDCMIMAGEAPPLYLOOKUPTABLE_H
