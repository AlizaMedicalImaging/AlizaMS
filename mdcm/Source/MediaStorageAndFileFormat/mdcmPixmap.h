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
 * Pixmap class
 * It does not contains any world space information.
 *
 */
class MDCM_EXPORT Pixmap : public Bitmap
{
public:
  Pixmap();
  ~Pixmap() = default;
  bool
  AreOverlaysInPixelData() const override;
  bool
  UnusedBitsPresentInPixelData() const override;
  Curve &       GetCurve(size_t = 0);
  const Curve & GetCurve(size_t = 0) const;
  size_t
  GetNumberOfCurves() const;
  void            SetNumberOfCurves(size_t);
  Overlay &       GetOverlay(size_t = 0);
  const Overlay & GetOverlay(size_t = 0) const;
  size_t
  GetNumberOfOverlays() const;
  void SetNumberOfOverlays(size_t);
  void RemoveOverlay(size_t);
  const IconImage &
  GetIconImage() const;
  IconImage &
  GetIconImage();
  void
  SetIconImage(const IconImage &);
  void
  Print(std::ostream &) const override;

protected:
  std::vector<Overlay>    Overlays;
  std::vector<Curve>      Curves;
  SmartPointer<IconImage> Icon;
};

} // end namespace mdcm

#endif // MDCMPIXMAP_H
