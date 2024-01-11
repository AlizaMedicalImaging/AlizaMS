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

#include "mdcmPixmapToPixmapFilter.h"
#include "mdcmPixmap.h"

namespace mdcm
{

Pixmap &
PixmapToPixmapFilter::GetInput()
{
  return dynamic_cast<Pixmap &>(*Input);
}

const Pixmap &
PixmapToPixmapFilter::GetOutput() const
{
  return dynamic_cast<const Pixmap &>(*Output);
}

const Pixmap &
PixmapToPixmapFilter::GetOutputAsPixmap() const
{
  return dynamic_cast<const Pixmap &>(*Output);
}

} // end namespace mdcm
