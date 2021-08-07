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
#ifndef MDCMJPEGCODEC_H
#define MDCMJPEGCODEC_H

#include "mdcmImageCodec.h"
#include <sstream>
/*
 * http://groups.google.com/group/comp.protocols.dicom/browse_thread/thread/625e46919f2080e1
 * http://groups.google.com/group/comp.protocols.dicom/browse_thread/thread/75fdfccc65a6243
 * http://groups.google.com/group/comp.protocols.dicom/browse_thread/thread/2d525ef6a2f093ed
 * http://groups.google.com/group/comp.protocols.dicom/browse_thread/thread/6b93af410f8c921f
 */

namespace mdcm
{

class PixelFormat;
class TransferSyntax;

class MDCM_EXPORT JPEGCodec : public ImageCodec
{
public:
  JPEGCodec();
  virtual ~JPEGCodec() override;
  bool
  CanDecode(TransferSyntax const &) const override;
  bool
  CanCode(TransferSyntax const &) const override;
  bool
  Decode(DataElement const &, DataElement &) override;
  bool
  Decode2(DataElement const &, std::stringstream &);
  bool
  Code(DataElement const &, DataElement &) override;
  void
  SetPixelFormat(PixelFormat const &) override;
  void
  ComputeOffsetTable(bool);
  virtual bool
  GetHeaderInfo(std::istream &, TransferSyntax &) override;
  void
  SetQuality(double);
  double
  GetQuality() const;
  void
  SetLossless(bool);
  bool
  GetLossless() const;
  virtual bool
  EncodeBuffer(std::ostream &, const char *, size_t);

protected:
  bool
  StartEncode(std::ostream &) override;
  bool
  StopEncode(std::ostream &) override;
  bool
  DecodeExtent(char *,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               unsigned int,
               std::istream &);
  virtual bool
  DecodeByStreams(std::istream &, std::ostream &) override;
  virtual bool
  InternalCode(const char *, size_t, std::ostream &);
  bool
  IsValid(PhotometricInterpretation const &) override;
  bool
  IsRowEncoder() override;
  bool
  IsFrameEncoder() override;
  bool
  AppendRowEncode(std::ostream &, const char *, size_t) override;
  bool
  AppendFrameEncode(std::ostream &, const char *, size_t) override;
  // Internal method called by SetPixelFormat
  // Instantiate the right jpeg codec (8, 12 or 16)
  void
  SetBitSample(int);
  virtual bool
  IsStateSuspension() const;
  int BitSample;
  int Quality;

private:
  void
  SetupJPEGBitCodec(int);
  JPEGCodec * Internal;
};

} // end namespace mdcm

#endif // MDCMJPEGCODEC_H
