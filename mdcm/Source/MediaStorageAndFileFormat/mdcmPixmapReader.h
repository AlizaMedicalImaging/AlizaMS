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
#ifndef MDCMPIXMAPREADER_H
#define MDCMPIXMAPREADER_H

#include "mdcmReader.h"
#include "mdcmPixmap.h"

namespace mdcm
{

class ByteValue;
class MediaStorage;
/**
 * PixmapReader
 * 
 * By default it is also loading the lookup table and overlay when found as
 * they impact the rendering or the image
 *
 * See PS 3.3-2008, Table C.7-11b IMAGE PIXEL MACRO ATTRIBUTES for the list of
 * attribute that belong to what mdcm calls a 'Pixmap'
 *
 */
class MDCM_EXPORT PixmapReader : public Reader
{
public:
  PixmapReader();
  virtual ~PixmapReader();
  void SetApplySupplementalLUT(bool);
  bool GetApplySupplementalLUT() const;
  void SetProcessOverlays(bool);
  bool GetProcessOverlays() const;
  void SetProcessIcons(bool);
  bool GetProcessIcons() const;
  void SetProcessCurves(bool);
  bool GetProcessCurves() const;
  const Pixmap & GetPixmap() const;
  Pixmap & GetPixmap();
  virtual bool Read() override;
protected:
  bool ReadImageInternal(const MediaStorage &, bool handlepixeldata = true);
  virtual bool ReadImage(const MediaStorage &);
  virtual bool ReadACRNEMAImage();
  SmartPointer<Pixmap> PixelData;
  bool m_AlppySupplementalLUT;
  bool m_ProcessOverlays;
  bool m_ProcessIcons;
  bool m_ProcessCurves;
};

} // end namespace mdcm

#endif //MDCMPIXMAPREADER_H
