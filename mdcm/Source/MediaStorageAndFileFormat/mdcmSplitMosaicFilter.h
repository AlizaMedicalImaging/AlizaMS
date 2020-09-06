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
#ifndef MDCMSPLITMOSAICFILTER_H
#define MDCMSPLITMOSAICFILTER_H

#include "mdcmFile.h"
#include "mdcmImage.h"

namespace mdcm
{

/*
 * Everything done in this code is for the sole purpose of writing interoperable
 * software under Sect. 1201 (f) Reverse Engineering exception of the DMCA.
 * If you believe anything in this code violates any law or any of your rights,
 * please contact us (mdcm-developers@lists.sourceforge.net) so that we can
 * find a solution.
 */
/**
 * \brief SplitMosaicFilter class
 * \details Class to reshuffle bytes for a SIEMENS Mosaic image
 * Siemens CSA Image Header
 * CSA:= Common Siemens Architecture, sometimes also known as Common syngo Architecture
 *
 */
class MDCM_EXPORT SplitMosaicFilter
{
public:
  SplitMosaicFilter();
  ~SplitMosaicFilter();

  /// Split the SIEMENS MOSAIC image
  bool Split();

  /// Compute the new dimensions according to private information
  /// stored in the MOSAIC header.
  bool ComputeMOSAICDimensions(unsigned int dims[3]);

  /// Extract the value for SliceNormalVector (CSA header)
  bool ComputeMOSAICSliceNormal( double dims[3], bool & inverted );

  /// Extract the value for ImagePositionPatient (requires inverted flag)
  bool ComputeMOSAICSlicePosition( double pos[3], bool inverted );

  void SetImage(const Image& image);
  const Image &GetImage() const { return *I; }
  Image &GetImage() { return *I; }

  void SetFile(const File& f) { F = f; }
  File &GetFile() { return *F; }
  const File &GetFile() const { return *F; }

protected:

private:
  SmartPointer<File> F;
  SmartPointer<Image> I;
};

} // end namespace mdcm

#endif //MDCMSPLITMOSAICFILTER_H
