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
  friend class ImageRegionReader;

  friend class Bitmap;

public:
  JPEG2000Codec();
  ~JPEG2000Codec();
  bool CanDecode(TransferSyntax const &) const;
  bool CanCode(TransferSyntax const &) const;
  bool Decode(DataElement const &, DataElement &);
  bool Code(DataElement const &, DataElement &);
  virtual bool GetHeaderInfo(std::istream &, TransferSyntax &);
  virtual ImageCodec * Clone() const;
  void SetRate(unsigned int, double);
  double GetRate(unsigned int = 0) const;
  void SetQuality(unsigned int, double);
  double GetQuality(unsigned int = 0) const;
  void SetTileSize(unsigned int, unsigned int);
  void SetNumberOfResolutions(unsigned int);
  void SetReversible(bool);

protected:
  bool DecodeExtent(
    char *,
    unsigned int, unsigned int,
    unsigned int, unsigned int,
    unsigned int, unsigned int,
    std::istream &);
  bool DecodeByStreams(std::istream &, std::ostream &);
  bool StartEncode(std::ostream &);
  bool IsRowEncoder();
  bool IsFrameEncoder();
  bool AppendRowEncode(std::ostream &, const char *, size_t);
  bool AppendFrameEncode(std::ostream &, const char *, size_t);
  bool StopEncode(std::ostream &);

private:
  std::pair<char *, size_t> DecodeByStreamsCommon(char *, size_t);
  bool CodeFrameIntoBuffer(char *, size_t, size_t &, const char *, size_t);
  bool GetHeaderInfo(const char *, size_t, TransferSyntax &);
  JPEG2000Internals * Internals;
};

} // end namespace mdcm

#endif //MDCMJPEG2000CODEC_H
