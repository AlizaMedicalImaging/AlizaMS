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
  See Copyright.txt or http://mdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMJPEGLSCODEC_H
#define MDCMJPEGLSCODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

class JPEGLSInternals;

/**
 * JPEG-LS
 * CharLS JPEG-LS implementation https://github.com/team-charls/charls
 */
class MDCM_EXPORT JPEGLSCodec : public ImageCodec
{
public:
  JPEGLSCodec();
  ~JPEGLSCodec() override;
  bool
  CanDecode(TransferSyntax const &) const override;
  bool
  CanCode(TransferSyntax const &) const override;
  bool
  Decode(DataElement const &, DataElement &) override;
  bool
  Decode2(DataElement const &, char *, size_t);
  bool
  Code(DataElement const &, DataElement &) override;
  unsigned long long
  GetBufferLength() const;
  void
  SetBufferLength(unsigned long long);
  bool
  GetHeaderInfo(std::istream &, TransferSyntax &) override;
  void
  SetLossless(bool);
  bool
  GetLossless() const;
  void
  SetLossyError(int); // [0-3] generally
protected:
  bool
  DecodeExtent(char *,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               std::istream &);
  bool
  StartEncode(std::ostream &) override;
  bool
  IsRowEncoder() override;
  bool
  IsFrameEncoder() override;
  bool
  AppendRowEncode(std::ostream &, const char *, size_t) override;
  bool
  AppendFrameEncode(std::ostream &, const char *, size_t) override;
  bool
  StopEncode(std::ostream &) override;

private:
  bool
  DecodeByStreamsCommon(const char *, size_t, std::vector<unsigned char> &);
  bool
  DecodeByStreamsCommon2(const char *, size_t, char *, size_t);
  bool
  CodeFrameIntoBuffer(char *, size_t, size_t &, const char *, size_t);
  unsigned long long BufferLength;
  int                LossyError;
};

} // end namespace mdcm

#endif // MDCMJPEGLSCODEC_H
