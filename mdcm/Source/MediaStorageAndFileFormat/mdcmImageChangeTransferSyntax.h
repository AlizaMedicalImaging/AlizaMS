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
#ifndef MDCMIMAGECHANGETRANSFERSYNTAX_H
#define MDCMIMAGECHANGETRANSFERSYNTAX_H

#include "mdcmImageToImageFilter.h"
#include "mdcmTransferSyntax.h"

namespace mdcm
{

class DataElement;
class ImageCodec;

/**
 * ImageChangeTransferSyntax class
 * Class to change the transfer syntax of an input DICOM
 *
 * If only Force param is set but no input TransferSyntax is set, it is assumed
 * that user only wants to inspect encapsulated stream (advanced dev. option).
 *
 * When using UserCodec it is very important that the TransferSyntax (as set in
 * SetTransferSyntax) is actually understood by UserCodec (ie.
 * UserCodec->CanCode(TransferSyntax)). Otherwise the behavior is to use a
 * default codec.
 *
 */

class MDCM_EXPORT ImageChangeTransferSyntax : public ImageToImageFilter
{
public:
  ImageChangeTransferSyntax()
    : TS(TransferSyntax::TS_END)
    , Force(false)
    , CompressIconImage(false)
    , ForceYBRFull(false)
    , UserCodec(nullptr)
  {}
  ~ImageChangeTransferSyntax() {}
  void
  SetTransferSyntax(const TransferSyntax &);
  const TransferSyntax &
  GetTransferSyntax() const;
  void
  SetCompressIconImage(bool);
  void
  SetForce(bool);
  void
  SetUserCodec(ImageCodec *);
  void
  SetForceYBRFull(bool);
  bool
  Change();

protected:
  bool
  TryRAWCodec(const DataElement &, Bitmap const &, Bitmap &);
  bool
  TryEncapsulatedRAWCodec(const DataElement &, Bitmap const &, Bitmap &);
  bool
  TryRLECodec(const DataElement &, Bitmap const &, Bitmap &);
  bool
  TryJPEGCodec(const DataElement &, Bitmap const &, Bitmap &);
  bool
  TryJPEGLSCodec(const DataElement &, Bitmap const &, Bitmap &);
  bool
  TryJPEG2000Codec(const DataElement &, Bitmap const &, Bitmap &);

private:
  TransferSyntax TS;
  bool           Force;
  bool           CompressIconImage;
  bool           ForceYBRFull;
  ImageCodec *   UserCodec;
};

} // end namespace mdcm

#endif // MDCMIMAGECHANGETRANSFERSYNTAX_H
