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
#ifndef MDCMJPEG16CODEC_H
#define MDCMJPEG16CODEC_H

#include "mdcmJPEGCodec.h"

namespace mdcm
{

class JPEGInternals_16BIT;
class ByteValue;

/**
 * Class for JPEG 16 bits (lossless)
 */
class JPEG16Codec : public JPEGCodec
{
public:
  JPEG16Codec();
  ~JPEG16Codec() override;
  bool DecodeByStreams(std::istream &, std::ostream &) override;
  bool InternalCode(const char *, size_t, std::ostream &) override;
  bool GetHeaderInfo(std::istream &, TransferSyntax &) override;

protected:
  bool IsStateSuspension() const override;
  bool EncodeBuffer(std::ostream &, const char *, size_t) override;

private:
  JPEGInternals_16BIT * Internals;
};

} // end namespace mdcm

#endif //MDCMJPEG16CODEC_H
