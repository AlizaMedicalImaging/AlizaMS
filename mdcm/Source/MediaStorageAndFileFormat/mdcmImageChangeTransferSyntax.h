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
 * \brief ImageChangeTransferSyntax class
 * \details Class to change the transfer syntax of an input DICOM
 *
 * If only Force param is set but no input TransferSyntax is set, it is assumed
 * that user only wants to inspect encapsulated stream (advanced dev. option).
 *
 * When using UserCodec it is very important that the TransferSyntax (as set in
 * SetTransferSyntax) is actually understood by UserCodec (ie.
 * UserCodec->CanCode( TransferSyntax ) ). Otherwise the behavior is to use a
 * default codec.
 *
 * \sa JPEGCodec JPEGLSCodec JPEG2000Codec
 */

class MDCM_EXPORT ImageChangeTransferSyntax : public ImageToImageFilter
{
public:
  ImageChangeTransferSyntax()
    :
    TS(TransferSyntax::TS_END),
    Force(false),
    CompressIconImage(false),
    ForceYBRFull(false),
	UserCodec(NULL) {}
  ~ImageChangeTransferSyntax() {}
  void SetTransferSyntax(const TransferSyntax & ts) { TS = ts; }
  const TransferSyntax & GetTransferSyntax() const { return TS; }
  bool Change();
  void SetCompressIconImage(bool b) { CompressIconImage = b; }
  // When target Transfer Syntax is identical to input target syntax, no
  // operation is actually done.
  // This is an issue when someone wants to re-compress using MDCM internal
  // implementation a JPEG (for example) image
  void SetForce( bool f ) { Force = f; }
  // Allow user to specify exactly which codec to use. this is needed to
  // specify special qualities or compression option.
  // warning: if the codec 'ic' is not compatible with the TransferSyntax
  // requested, it will not be used. It is the user responsibility to check
  // that UserCodec->CanCode( TransferSyntax )
  void SetUserCodec(ImageCodec *ic) { UserCodec = ic; }
  void SetForceYBRFull(bool t) { ForceYBRFull = t; }

protected:
  bool TryJPEGCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output);
  bool TryJPEG2000Codec(const DataElement & pixelde, Bitmap const & input, Bitmap & output);
  bool TryJPEGLSCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output);
  bool TryRAWCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output);
  bool TryRLECodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output);

private:
  TransferSyntax TS;
  bool Force;
  bool CompressIconImage;
  bool ForceYBRFull;
  ImageCodec *UserCodec;
};

} // end namespace mdcm

#endif //MDCMIMAGECHANGETRANSFERSYNTAX_H
