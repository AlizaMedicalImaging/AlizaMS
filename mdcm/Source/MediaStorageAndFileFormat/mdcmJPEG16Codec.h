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
 * \brief Class to do JPEG 16bits (lossless)
 * \note internal class
 */
class JPEG16Codec : public JPEGCodec
{
public:
  JPEG16Codec();
  ~JPEG16Codec();

  bool DecodeByStreams(std::istream &is, std::ostream &os);
  bool InternalCode(const char *input, unsigned long len, std::ostream &os);

  bool GetHeaderInfo(std::istream &is, TransferSyntax &ts);

protected:
  bool IsStateSuspension() const;
  virtual bool EncodeBuffer(std::ostream &os, const char *data, size_t datalen);

private:
  JPEGInternals_16BIT *Internals;
};

} // end namespace mdcm

#endif //MDCMJPEG16CODEC_H
