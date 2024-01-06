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
#ifndef MDCMJPEG2000CODEC_H
#define MDCMJPEG2000CODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

class JPEG2000Internals;
/*
 * the class will produce JPC (JPEG 2000 codestream), since some private implementor
 * are using full jp2 file the decoder tolerate jp2 input
 * this is an implementation of an ImageCodec
 */
class MDCM_EXPORT JPEG2000Codec : public ImageCodec
{
  friend class Bitmap;

public:
  JPEG2000Codec();
  ~JPEG2000Codec() override;
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
  bool
  GetHeaderInfo(std::istream &) override;
  void
  SetRate(unsigned int, double);
  double
  GetRate(unsigned int = 0) const;
  void
  SetQuality(unsigned int, double);
  double
  GetQuality(unsigned int = 0) const;
  void
  SetTileSize(unsigned int, unsigned int);
  void
  SetNumberOfResolutions(unsigned int);
  void
  SetReversible(bool);
  void
  SetMCT(bool);
  bool
  GetReversible() const;
  bool
  GetMCT() const;

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
  std::pair<char *, size_t>
  DecodeByStreamsCommon(char *, size_t);
  bool
  CodeFrameIntoBuffer(char *, size_t, size_t &, const char *, size_t);
  bool
  GetHeaderInfo(const char *, size_t);
  JPEG2000Internals * Internals;
};

} // end namespace mdcm

#endif // MDCMJPEG2000CODEC_H
