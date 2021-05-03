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
 * SplitMosaicFilter class
 * Class to reshuffle bytes for a SIEMENS Mosaic image
 * Siemens CSA Image Header
 * CSA:= Common Siemens Architecture, sometimes also known as Common syngo Architecture
 *
 */
class MDCM_EXPORT SplitMosaicFilter
{
public:
  SplitMosaicFilter();
  ~SplitMosaicFilter();
  bool
  Split();
  bool
  ComputeMOSAICDimensions(unsigned int[3]);
  bool
  ComputeMOSAICSliceNormal(double[3], bool &);
  bool
  ComputeMOSAICSlicePosition(double[3], bool);
  void
  SetImage(const Image &);
  const Image &
  GetImage() const
  {
    return *I;
  }
  Image &
  GetImage()
  {
    return *I;
  }
  void
  SetFile(const File & f)
  {
    F = f;
  }
  File &
  GetFile()
  {
    return *F;
  }
  const File &
  GetFile() const
  {
    return *F;
  }

private:
  SmartPointer<File>  F;
  SmartPointer<Image> I;
};

} // end namespace mdcm

#endif // MDCMSPLITMOSAICFILTER_H
