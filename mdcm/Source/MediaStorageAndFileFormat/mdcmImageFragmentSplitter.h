/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMIMAGEFRAGMENTSPLITTER_H
#define MDCMIMAGEFRAGMENTSPLITTER_H

#include "mdcmImageToImageFilter.h"

namespace mdcm
{

class DataElement;
/**
 * \brief ImageFragmentSplitter class
 * \details For single frame image, DICOM standard allow splitting the frame into multiple fragments
 */
class MDCM_EXPORT ImageFragmentSplitter : public ImageToImageFilter
{
public:
  ImageFragmentSplitter() : FragmentSizeMax(0), Force(false) {}
  ~ImageFragmentSplitter() {}
  bool Split();
  void SetFragmentSizeMax(unsigned int fragsize);
  unsigned int GetFragmentSizeMax() const { return FragmentSizeMax; }
  void SetForce(bool f) { Force = f; }

private:
  unsigned int FragmentSizeMax;
  bool Force;
};

} // end namespace mdcm

#endif //MDCMIMAGEFRAGMENTSPLITTER_H
