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
#ifndef MDCMIMAGECODEC_H
#define MDCMIMAGECODEC_H

#include "mdcmTypes.h"
#include "mdcmSmartPointer.h"
#include "mdcmDataElement.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmLookupTable.h"
#include "mdcmPixelFormat.h"

namespace mdcm
{

/**
 * ImageCodec
 */
class MDCM_EXPORT ImageCodec
{
  friend class FileChangeTransferSyntax;

public:
  ImageCodec();
  virtual ~ImageCodec();
  virtual bool
  CanDecode(TransferSyntax const &) const;
  virtual bool
  Decode(DataElement const &, DataElement &);
  virtual bool
  CanCode(TransferSyntax const &) const;
  virtual bool
  Code(DataElement const &, DataElement &);
  bool
  IsLossy() const;
  void
  SetLossyFlag(bool);
  bool
  GetLossyFlag() const;
  virtual bool
  GetHeaderInfo(std::istream &, TransferSyntax &);
  unsigned int
  GetPlanarConfiguration() const;
  void
  SetPlanarConfiguration(unsigned int);
  PixelFormat &
  GetPixelFormat();
  const PixelFormat &
  GetPixelFormat() const;
  virtual void
  SetPixelFormat(PixelFormat const &);
  const PhotometricInterpretation &
  GetPhotometricInterpretation() const;
  void
  SetPhotometricInterpretation(PhotometricInterpretation const &);
  bool
  GetNeedByteSwap() const;
  void
  SetNeedByteSwap(bool);
  void
  SetNeedOverlayCleanup(bool);
  void
  SetLUT(LookupTable const &);
  const LookupTable &
  GetLUT() const;
  void
  SetDimensions(const unsigned int[3]);
  void
  SetDimensions(const std::vector<unsigned int> &);
  const unsigned int *
  GetDimensions() const;
  void
  SetNumberOfDimensions(unsigned int);
  unsigned int
  GetNumberOfDimensions() const;
  bool
  CleanupUnusedBits(char *, size_t);

protected:
  virtual bool
  IsValid(PhotometricInterpretation const &);
  virtual bool
  IsRowEncoder();
  virtual bool
  IsFrameEncoder();
  virtual bool
  StartEncode(std::ostream &);
  virtual bool
  AppendRowEncode(std::ostream &, const char *, size_t);
  virtual bool
  AppendFrameEncode(std::ostream &, const char *, size_t);
  virtual bool
  StopEncode(std::ostream &);
  virtual bool
  DecodeByStreams(std::istream &, std::ostream &);
  bool
  DoByteSwap(std::istream &, std::ostream &);
  bool
  DoYBRFull422(std::istream &, std::ostream &);
  bool
  DoPlanarConfiguration(std::istream &, std::ostream &);
  bool
  DoSimpleCopy(std::istream &, std::ostream &);
  bool
  DoPaddedCompositePixelCode(std::istream &, std::ostream &);
  bool
  DoInvertMonochrome(std::istream &, std::ostream &);
  bool
  DoOverlayCleanup(std::istream &, std::ostream &);
  bool                              RequestPlanarConfiguration;
  bool                              RequestPaddedCompositePixelCode;
  unsigned int                      PlanarConfiguration;
  PhotometricInterpretation         PI;
  PixelFormat                       PF;
  bool                              NeedByteSwap;
  bool                              NeedOverlayCleanup;
  typedef SmartPointer<LookupTable> LUTPtr;
  LUTPtr                            LUT;
  unsigned int                      Dimensions[3];
  unsigned int                      NumberOfDimensions;
  bool                              LossyFlag;
};

} // end namespace mdcm

#endif // MDCMIMAGECODEC_H
