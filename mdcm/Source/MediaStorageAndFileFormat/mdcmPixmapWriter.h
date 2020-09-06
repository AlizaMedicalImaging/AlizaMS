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
 * \brief PixmapWriter
 * \details This class will takes two inputs:
 * 1. The DICOM DataSet
 * 2. The Image input
 * It will override any info from the Image over the DataSet.
 *
 * For instance when one read in a lossy compressed image and write out as unencapsulated
 * (ie implicitely lossless) then some attribute are definitely needed to mark this
 * dataset as Lossy (typically 0028,2114)
 */
class MDCM_EXPORT PixmapWriter : public Writer
{
public:
  PixmapWriter();
  ~PixmapWriter();
  const Pixmap & GetPixmap() const { return *PixelData; }
  Pixmap & GetPixmap() { return *PixelData; } // FIXME
  void SetPixmap(Pixmap const &img);
  // It will overwrite anything Pixmap infos found in DataSet
  // (see parent class to see how to pass dataset)
  virtual const Pixmap & GetImage() const { return *PixelData; }
  virtual Pixmap & GetImage() { return *PixelData; } // FIXME
  virtual void SetImage(Pixmap const & img);
  bool Write();

protected:
  void DoIconImage(DataSet & ds, Pixmap const & image);
  bool PrepareWrite( MediaStorage const & refms );
  SmartPointer<Pixmap> PixelData;
};

} // end namespace mdcm

#endif //MDCMPIXMAPWRITER_H
