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
#ifndef MDCMPIXMAPWRITER_H
#define MDCMPIXMAPWRITER_H

#include "mdcmWriter.h"
#include "mdcmPixmap.h"

namespace mdcm
{

class StreamImageWriter;
class Pixmap;
/**
 * PixmapWriter
 *
 * This class will takes two inputs:
 * 1. The DICOM DataSet
 * 2. The Image input
 *
 * It will override any info from the Image over the DataSet.
 *
 * For instance when one read in a lossy compressed image and
 * write out as unencapsulated (ie implicitely lossless) then
 * some attribute are definitely needed to mark this dataset
 * as Lossy (typically 0028,2114).
 */
class MDCM_EXPORT PixmapWriter : public Writer
{
public:
  PixmapWriter();
  ~PixmapWriter() = default;
  const Pixmap &
  GetPixmap() const;
  Pixmap &
  GetPixmap();
  void
  SetPixmap(Pixmap const &);
  const Pixmap &
  GetImage() const;
  Pixmap &
  GetImage();
  void
  SetImage(Pixmap const &);
  bool
  Write();

protected:
  bool
  PrepareWrite(MediaStorage const &);
  void
  DoIconImage(DataSet &, Pixmap const &);
  SmartPointer<Pixmap> PixelData;

private:

};

} // end namespace mdcm

#endif // MDCMPIXMAPWRITER_H
