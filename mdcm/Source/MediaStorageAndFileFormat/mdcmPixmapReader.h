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
 * \brief PixmapReader
 * \details 
 * \note its role is to convert the DICOM DataSet into a Pixmap
 * representation
 * By default it is also loading the lookup table and overlay when found as
 * they impact the rendering or the image
 *
 * See PS 3.3-2008, Table C.7-11b IMAGE PIXEL MACRO ATTRIBUTES for the list of
 * attribute that belong to what mdcm calls a 'Pixmap'
 *
 * \warning the API ReadUpToTag and ReadSelectedTag
 *
 * \see Pixmap
 */
class MDCM_EXPORT PixmapReader : public Reader
{
public:
  PixmapReader();
  virtual ~PixmapReader();
  void SetApplySupplementalLUT(bool t) { m_AlppySupplementalLUT = t; }
  bool GetApplySupplementalLUT() const { return m_AlppySupplementalLUT; }
  void SetProcessOverlays(bool t) { m_ProcessOverlays = t; }
  bool GetProcessOverlays() const { return m_ProcessOverlays; }
  void SetProcessIcons(bool t) { m_ProcessIcons = t; }
  bool GetProcessIcons() const { return m_ProcessIcons; }
  void SetProcessCurves(bool t) { m_ProcessCurves = t; }
  bool GetProcessCurves() const { return m_ProcessCurves; }
  virtual bool Read();
  // Following methods are valid only after a call to 'Read'
  const Pixmap& GetPixmap() const;
  Pixmap& GetPixmap();
protected:
  bool ReadImageInternal(MediaStorage const & ms, bool handlepixeldata = true);
  virtual bool ReadImage(MediaStorage const & ms);
  virtual bool ReadACRNEMAImage();
  SmartPointer<Pixmap> PixelData;
  bool m_AlppySupplementalLUT;
  bool m_ProcessOverlays;
  bool m_ProcessIcons;
  bool m_ProcessCurves;
};

} // end namespace mdcm

#endif //MDCMPIXMAPREADER_H
