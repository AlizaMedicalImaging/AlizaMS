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
#ifndef MDCMJPEG8CODEC_H
#define MDCMJPEG8CODEC_H

#include "mdcmJPEGCodec.h"

namespace mdcm
{

class JPEGInternals_8BIT;
class ByteValue;

/**
 * Class for JPEG 8 bits (lossy & lossless)
 */
class JPEG8Codec : public JPEGCodec
{
public:
  JPEG8Codec();
  ~JPEG8Codec() override;
  bool
  DecodeByStreams(std::istream &, std::ostream &) override;
  bool
  InternalCode(const char *, size_t, std::ostream &) override;
  bool
  GetHeaderInfo(std::istream &) override;
  bool
  GetHeaderInfoAndTS(std::istream &, TransferSyntax &);

protected:
  bool
  IsStateSuspension() const override;
  bool
  EncodeBuffer(std::ostream &, const char *, size_t) override;

private:
  JPEGInternals_8BIT * Internals;
};

} // end namespace mdcm

#endif // MDCMJPEG8CODEC_H
