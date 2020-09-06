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
 * \brief Class to do JPEG 8bits (lossy & lossless)
 * \note internal class
 */
class JPEG8Codec : public JPEGCodec
{
public:
  JPEG8Codec();
  ~JPEG8Codec();

  bool DecodeByStreams(std::istream &is, std::ostream &os);
  bool InternalCode(const char *input, unsigned long len, std::ostream &os);

  bool GetHeaderInfo(std::istream &is, TransferSyntax &ts);

protected:
  bool IsStateSuspension() const;
  virtual bool EncodeBuffer(std::ostream &os, const char *data, size_t datalen);

private:
  JPEGInternals_8BIT *Internals;
};

} // end namespace mdcm

#endif //MDCMJPEG8CODEC_H
