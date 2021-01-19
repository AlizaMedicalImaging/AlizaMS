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
#ifndef MDCMICONIMAGEFILTER_H
#define MDCMICONIMAGEFILTER_H

#include "mdcmFile.h"
#include "mdcmIconImage.h"

namespace mdcm
{
class IconImageFilterInternals;

class MDCM_EXPORT IconImageFilter
{
public:
  IconImageFilter();
  ~IconImageFilter();
  void SetFile(const File & f) { F = f; }
  File & GetFile() { return *F; }
  const File & GetFile() const { return *F; }
  bool Extract();
  unsigned int GetNumberOfIconImages() const;
  IconImage & GetIconImage(unsigned int) const;

protected:
  void ExtractIconImages();
  void ExtractVeproIconImages();

private:
  SmartPointer<File> F;
  IconImageFilterInternals * Internals;
};

} // end namespace mdcm

#endif //MDCMICONIMAGEFILTER_H
