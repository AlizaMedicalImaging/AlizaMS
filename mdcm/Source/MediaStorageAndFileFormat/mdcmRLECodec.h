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
#ifndef MDCMRLECODEC_H
#define MDCMRLECODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

class Fragment;
class RLEInternals;
/**
 * Class to do RLE
 *
 * ANSI X3.9
 * A.4.2 RLE Compression
 * Annex G defines a RLE Compression Transfer Syntax. This transfer Syntax is
 * identified by the UID value "1.2.840.10008.1.2.5". If the object allows
 * multi-frame images in the pixel data field, then each frame shall be encoded
 * separately. Each frame shall be encoded in one and only one Fragment (see PS
 * 3.5.8.2).
 *
 */
class MDCM_EXPORT RLECodec : public ImageCodec
{
public:
  RLECodec();
  ~RLECodec() override;
  bool
  CanCode(const TransferSyntax &) const override;
  bool
  CanDecode(const TransferSyntax &) const override;
  bool
  Decode(const DataElement &, DataElement &) override;
  bool
  Code(const DataElement &, DataElement & out) override;
  unsigned long long
  GetBufferLength() const;
  void
  SetBufferLength(unsigned long long);
  bool
  GetHeaderInfo(std::istream &) override;
  void
  SetLength(unsigned long long);

protected:
  bool
  DecodeByStreams(std::istream &, std::ostream &) override;
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
  size_t
  DecodeFragment(const Fragment &, char *, size_t);
  RLEInternals *     Internals;
  unsigned long long Length{};
  unsigned long long BufferLength{};
};

} // end namespace mdcm

#endif // MDCMRLECODEC_H
