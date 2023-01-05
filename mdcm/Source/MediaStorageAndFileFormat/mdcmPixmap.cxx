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

#include "mdcmPixmap.h"

namespace mdcm
{

Pixmap::Pixmap()
  : Overlays()
  , Curves()
  , Icon(new IconImage)
{}

Pixmap::~Pixmap() {}

bool
Pixmap::AreOverlaysInPixelData() const
{
  int                                  total = 0;
  std::vector<Overlay>::const_iterator it = Overlays.begin();
  for (; it != Overlays.end(); ++it)
  {
    total += static_cast<int>(it->IsInPixelData());
  }
  return total != 0;
}

bool
Pixmap::UnusedBitsPresentInPixelData() const
{
  const PixelFormat & pf = GetPixelFormat();
  unsigned short      ba = pf.GetBitsAllocated();
  unsigned short      bs = pf.GetBitsStored();
  return ba != bs;
}

Curve &
Pixmap::GetCurve(size_t i)
{
  assert(i < Curves.size());
  return Curves[i];
}

const Curve &
Pixmap::GetCurve(size_t i) const
{
  assert(i < Curves.size());
  return Curves[i];
}

size_t
Pixmap::GetNumberOfCurves() const
{
  return Curves.size();
}

void
Pixmap::SetNumberOfCurves(size_t n)
{
  Curves.resize(n);
}

Overlay &
Pixmap::GetOverlay(size_t i)
{
  assert(i < Overlays.size());
  return Overlays[i];
}

const Overlay &
Pixmap::GetOverlay(size_t i) const
{
  assert(i < Overlays.size());
  return Overlays[i];
}

size_t
Pixmap::GetNumberOfOverlays() const
{
  return Overlays.size();
}

void
Pixmap::SetNumberOfOverlays(size_t n)
{
  Overlays.resize(n);
}

void
Pixmap::RemoveOverlay(size_t i)
{
  assert(i < Overlays.size());
  Overlays.erase(Overlays.begin() + i);
}

const IconImage &
Pixmap::GetIconImage() const
{
  return *Icon;
}

IconImage &
Pixmap::GetIconImage()
{
  return *Icon;
}

void
Pixmap::SetIconImage(IconImage const & ii)
{
  Icon = ii;
}

void
Pixmap::Print(std::ostream & os) const
{
  Bitmap::Print(os);
  for (std::vector<Overlay>::const_iterator it = Overlays.begin(); it != Overlays.end(); ++it)
  {
    it->Print(os);
  }
  for (std::vector<Curve>::const_iterator it = Curves.begin(); it != Curves.end(); ++it)
  {
    it->Print(os);
  }
}

} // namespace mdcm
