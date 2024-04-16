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
#ifndef MDCMBITMAP_H
#define MDCMBITMAP_H

#include "mdcmObject.h"
#include "mdcmCurve.h"
#include "mdcmDataElement.h"
#include "mdcmLookupTable.h"
#include "mdcmOverlay.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmPixelFormat.h"
#include "mdcmTransferSyntax.h"
#include <vector>

namespace mdcm
{

/**
 * Bitmap class
 *
 * A bitmap based image. Used as parent for icon image and the main pixel image,
 * does not contains any world space information.
 */

class MDCM_EXPORT Bitmap : public Object
{
  friend class PixmapReader;
  friend class ImageChangeTransferSyntax;

public:
  Bitmap() = default;
  virtual ~Bitmap() = default;
  unsigned int
  GetNumberOfDimensions() const;
  void
  SetNumberOfDimensions(unsigned int);
  const unsigned int *
  GetDimensions() const;
  unsigned int
  GetDimension(unsigned int) const;
  void
  SetDimensions(const unsigned int[3]);
  void
  SetDimension(unsigned int, unsigned int);
  unsigned int
  GetPlanarConfiguration() const;
  void
  SetPlanarConfiguration(unsigned int);
  void
  ClearDimensions();
  bool
  IsDimensionEmpty() const;
  const PhotometricInterpretation &
  GetPhotometricInterpretation() const;
  void
  SetPhotometricInterpretation(const PhotometricInterpretation &);
  bool
  IsLossy() const;
  void
  SetLossyFlag(bool);
  unsigned long long
  GetBufferLength() const;
  bool
  GetBuffer(char *) const;
  virtual bool
  AreOverlaysInPixelData() const;
  virtual bool
  UnusedBitsPresentInPixelData() const;
  bool
  GetNeedByteSwap() const;
  void
  SetNeedByteSwap(bool);
  void
  SetTransferSyntax(const TransferSyntax &);
  const TransferSyntax &
  GetTransferSyntax() const;
  bool
  IsTransferSyntaxCompatible(const TransferSyntax &) const;
  void
  SetDataElement(const DataElement &);
  const DataElement &
  GetDataElement() const;
  DataElement &
  GetDataElement();
  void
  SetLUT(const LookupTable &);
  const LookupTable &
  GetLUT() const;
  LookupTable &
  GetLUT();
  void
  SetColumns(unsigned int);
  unsigned int
  GetColumns() const;
  void
  SetRows(unsigned int);
  unsigned int
  GetRows() const;
  const PixelFormat &
  GetPixelFormat() const;
  PixelFormat &
  GetPixelFormat();
  void
  SetPixelFormat(const PixelFormat &);
  void
  Print(std::ostream &) const;

protected:
  bool
  ComputeLossyFlag();
  bool
  TryRAWCodec(char *, bool &) const;
  bool
  TryEncapsulatedRAWCodec(char *, bool &) const;
  bool
  TryJPEGCodec(char *, bool &) const;
  bool
  TryJPEGLSCodec(char *, bool &) const;
  bool
  TryJPEG2000Codec(char *, bool &) const;
  bool
  TryRLECodec(char *, bool &) const;
  unsigned int              PlanarConfiguration{};
  unsigned int              NumberOfDimensions{2};
  TransferSyntax            TS{};
  PixelFormat               PF{};
  PhotometricInterpretation PI{};
  std::vector<unsigned int> Dimensions{0, 0, 1};
  DataElement               PixelData{};
  LookupTable               LUT{};
  bool                      NeedByteSwap{};
  bool                      LossyFlag{};

private:
  bool
  GetBufferInternal(char *, bool &) const;

};

} // end namespace mdcm

#endif // MDCMBITMAP_H
