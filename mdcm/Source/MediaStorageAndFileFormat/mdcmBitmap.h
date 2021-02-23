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
#include "mdcmSmartPointer.h"
#include "mdcmTransferSyntax.h"
#include <vector>

namespace mdcm
{

/**
 * Bitmap class
 * A bitmap based image. Used as parent for both IconImage and the main Pixel Data Image
 * It does not contains any World Space information (IPP, IOP)
 */

class MDCM_EXPORT Bitmap : public Object
{
public:
  Bitmap();
  ~Bitmap();
  void Print(std::ostream &) const;
  virtual bool AreOverlaysInPixelData() const { return false; }
  virtual bool UnusedBitsPresentInPixelData() const { return false; }
  unsigned int GetNumberOfDimensions() const;
  void SetNumberOfDimensions(unsigned int dim);
  unsigned int GetPlanarConfiguration() const;
  // Call SetPixelFormat first, before SetPlanarConfiguration!
  void SetPlanarConfiguration(unsigned int pc);
  bool GetNeedByteSwap() const
  {
    return NeedByteSwap;
  }
  void SetNeedByteSwap(bool b)
  {
    NeedByteSwap = b;
  }
  void SetTransferSyntax(TransferSyntax const & ts)
  {
    TS = ts;
  }
  const TransferSyntax & GetTransferSyntax() const
  {
    return TS;
  }
  bool IsTransferSyntaxCompatible(TransferSyntax const & ts) const;
  void SetDataElement(DataElement const & de)
  {
    PixelData = de;
  }
  const DataElement & GetDataElement() const { return PixelData; }
  DataElement& GetDataElement() { return PixelData; }

  void SetLUT(LookupTable const & lut)
  {
    LUT = SmartPointer<LookupTable>(const_cast<LookupTable*>(&lut));
  }
  const LookupTable & GetLUT() const
  {
    return *LUT;
  }
  LookupTable & GetLUT()
  {
    return *LUT;
  }
  const unsigned int * GetDimensions() const;
  unsigned int GetDimension(unsigned int idx) const;
  void SetColumns(unsigned int col) { SetDimension(0,col); }
  unsigned int GetColumns() const { return GetDimension(0); }
  void SetRows(unsigned int rows) { SetDimension(1,rows); }
  unsigned int GetRows() const { return GetDimension(1); }
  void SetDimensions(const unsigned int dims[3]);
  void SetDimension(unsigned int idx, unsigned int dim);
  const PixelFormat & GetPixelFormat() const
  {
    return PF;
  }
  PixelFormat & GetPixelFormat()
  {
    return PF;
  }
  void SetPixelFormat(PixelFormat const &pf)
  {
    PF = pf;
    PF.Validate();
  }
  const PhotometricInterpretation & GetPhotometricInterpretation() const;
  void SetPhotometricInterpretation(PhotometricInterpretation const & pi);
  bool IsEmpty() const { return Dimensions.size() == 0; }
  void Clear();
  // WARNING for palette color: multiply this length by 3
  // if computing the size of equivalent RGB image
  unsigned long long GetBufferLength() const;
  bool GetBuffer(char * buffer) const;
  bool IsLossy() const;
  void SetLossyFlag(bool f) { LossyFlag = f; }

protected:
  bool TryRAWCodec     (char * buffer, bool & lossyflag) const;
  bool TryJPEGCodec    (char * buffer, bool & lossyflag) const;
  bool TryPVRGCodec    (char * buffer, bool & lossyflag) const;
  bool TryJPEGLSCodec  (char * buffer, bool & lossyflag) const;
  bool TryJPEG2000Codec(char * buffer, bool & lossyflag) const;
  bool TryRLECodec     (char * buffer, bool & lossyflag) const;
  bool TryJPEGCodec2    (std::ostream & os) const;
  bool TryJPEG2000Codec2(std::ostream & os) const;
  bool GetBuffer2       (std::ostream & os) const;
  friend class PixmapReader;
  friend class ImageChangeTransferSyntax;
  // WARNING: image can be lossy but in implicit little endian format
  bool ComputeLossyFlag();

protected:
  unsigned int PlanarConfiguration;
  unsigned int NumberOfDimensions;
  TransferSyntax TS;
  PixelFormat PF;
  PhotometricInterpretation PI;
  std::vector<unsigned int> Dimensions;
  DataElement PixelData;
  typedef SmartPointer<LookupTable> LUTPtr;
  LUTPtr LUT;
  bool NeedByteSwap;
  bool LossyFlag;

private:
  bool GetBufferInternal(char * buffer, bool & lossyflag) const;
};

} // end namespace mdcm

#endif // MDCMBITMAP_H
