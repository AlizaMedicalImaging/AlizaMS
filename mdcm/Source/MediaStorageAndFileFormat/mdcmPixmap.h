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
#ifndef MDCMPIXMAP_H
#define MDCMPIXMAP_H

#include "mdcmBitmap.h"
#include "mdcmCurve.h"
#include "mdcmIconImage.h"
#include "mdcmOverlay.h"

namespace mdcm
{

/**
 * \brief Pixmap class
 * \details A bitmap based image. Used as parent for both IconImage and the main Pixel Data Image
 * It does not contains any World Space information (IPP, IOP)
 *
 * \see PixmapReader
 */
class MDCM_EXPORT Pixmap : public Bitmap
{
public:

  Pixmap();
  ~Pixmap();
  void Print(std::ostream &) const;

  bool AreOverlaysInPixelData() const;
  bool UnusedBitsPresentInPixelData() const;

  Curve & GetCurve(size_t i = 0)
  {
    assert( i < Curves.size() );
    return Curves[i];
  }

  const Curve & GetCurve(size_t i = 0) const {
    assert( i < Curves.size() );
    return Curves[i];
  }

  size_t GetNumberOfCurves() const { return Curves.size(); }
  void SetNumberOfCurves(size_t n) { Curves.resize(n); }

  Overlay & GetOverlay(size_t i = 0)
  {
    assert( i < Overlays.size() );
    return Overlays[i];
  }

  const Overlay & GetOverlay(size_t i = 0) const
  {
    assert( i < Overlays.size() );
    return Overlays[i];
  }

  size_t GetNumberOfOverlays() const { return Overlays.size(); }
  void SetNumberOfOverlays(size_t n) { Overlays.resize(n); }

  void RemoveOverlay(size_t i)
  {
    assert( i < Overlays.size() );
    Overlays.erase( Overlays.begin() + i );
  }

  const IconImage & GetIconImage() const { return *Icon; }
  IconImage & GetIconImage() { return *Icon; }
  void SetIconImage(IconImage const & ii) { Icon = ii; }

protected:
  std::vector<Overlay>  Overlays;
  std::vector<Curve>  Curves;
  SmartPointer<IconImage> Icon;
};

} // end namespace mdcm

#endif //MDCMPIXMAP_H
