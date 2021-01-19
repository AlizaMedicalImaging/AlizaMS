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
friend class ImageRegionReader;
public:
  JPEGLSCodec();
  ~JPEGLSCodec();
  bool CanDecode(TransferSyntax const &) const;
  bool CanCode(TransferSyntax const &) const;
  unsigned long long GetBufferLength() const { return BufferLength; }
  void SetBufferLength(unsigned long long l) { BufferLength = l; }
  bool Decode(DataElement const &, DataElement &);
  bool Decode(DataElement const &, char *, size_t,
    uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t);
  bool Code(DataElement const &, DataElement &);
  bool GetHeaderInfo(std::istream &, TransferSyntax &);
  ImageCodec * Clone() const;
  void SetLossless(bool);
  bool GetLossless() const;
  void SetLossyError(int); // [0-3] generally

protected:
  bool DecodeExtent(char *,
    unsigned int, unsigned int,
    unsigned int, unsigned int,
    unsigned int, unsigned int,
    std::istream &);
  bool StartEncode(std::ostream &);
  bool IsRowEncoder();
  bool IsFrameEncoder();
  bool AppendRowEncode(std::ostream &, const char *, size_t);
  bool AppendFrameEncode(std::ostream &, const char *, size_t);
  bool StopEncode(std::ostream &);

private:
  bool DecodeByStreamsCommon(const char *, size_t, std::vector<unsigned char> &);
  bool CodeFrameIntoBuffer(char *, size_t, size_t &, const char *, size_t);
  unsigned long long BufferLength;
  int LossyError;
};

} // end namespace mdcm

#endif //MDCMJPEGLSCODEC_H
