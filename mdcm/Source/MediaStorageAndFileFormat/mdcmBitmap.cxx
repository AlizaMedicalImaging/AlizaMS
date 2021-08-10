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

#include "mdcmBitmap.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmRAWCodec.h"
#include "mdcmJPEGCodec.h"
#include "mdcmPVRGCodec.h"
#include "mdcmJPEGLSCodec.h"
#include "mdcmJPEG2000Codec.h"
#include "mdcmRLECodec.h"
#include "mdcmImageHelper.h"
#include <cstring>

namespace mdcm
{

Bitmap::Bitmap()
  : PlanarConfiguration(0)
  , NumberOfDimensions(2)
  , TS()
  , PF()
  , PI()
  , Dimensions()
  , PixelData()
  , LUT(new LookupTable)
  , NeedByteSwap(false)
  , LossyFlag(false)
{}

Bitmap::~Bitmap() {}

unsigned int
Bitmap::GetNumberOfDimensions() const
{
  return NumberOfDimensions;
}

void
Bitmap::SetNumberOfDimensions(unsigned int dim)
{
  NumberOfDimensions = dim;
  assert(NumberOfDimensions);
  Dimensions.resize(3);
  assert(NumberOfDimensions == 2 || NumberOfDimensions == 3);
  if (NumberOfDimensions == 2)
  {
    Dimensions[2] = 1;
  }
}

const unsigned int *
Bitmap::GetDimensions() const
{
  assert(NumberOfDimensions);
  return &Dimensions[0];
}

unsigned int
Bitmap::GetDimension(unsigned int idx) const
{
  assert(NumberOfDimensions);
  return Dimensions[idx];
}

void
Bitmap::SetDimensions(const unsigned int * dims)
{
  assert(NumberOfDimensions);
  assert(Dimensions.size() == 3);
  Dimensions[0] = dims[0];
  Dimensions[1] = dims[1];
  if (NumberOfDimensions == 2)
    Dimensions[2] = 1;
  else
    Dimensions[2] = dims[2];
}

void
Bitmap::SetDimension(unsigned int idx, unsigned int dim)
{
  assert(NumberOfDimensions);
  assert(idx < NumberOfDimensions);
  Dimensions.resize(3);
  Dimensions[idx] = dim;
  if (NumberOfDimensions == 2)
    Dimensions[2] = 1;
}

unsigned int
Bitmap::GetPlanarConfiguration() const
{
  if (PlanarConfiguration && PF.GetSamplesPerPixel() != 3)
  {
    assert(0);
    mdcmWarningMacro("Can't set PlanarConfiguration if SamplesPerPixel is not 3");
    return 0;
  }
  return PlanarConfiguration;
}

void
Bitmap::SetPlanarConfiguration(unsigned int pc)
{
  assert(pc == 0 || pc == 1);
  PlanarConfiguration = pc;
  if (pc)
  {
    if (PF.GetSamplesPerPixel() != 3)
    {
      mdcmWarningMacro("Cant have Planar Configuration in non RGB input. Discarding");
      PlanarConfiguration = 0;
    }
    const TransferSyntax & ts = GetTransferSyntax();
    if (ts == TransferSyntax::JPEGBaselineProcess1 || ts == TransferSyntax::JPEGExtendedProcess2_4 ||
        ts == TransferSyntax::JPEGExtendedProcess3_5 || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
        ts == TransferSyntax::JPEGFullProgressionProcess10_12 || ts == TransferSyntax::JPEGLosslessProcess14 ||
        ts == TransferSyntax::JPEGLosslessProcess14_1 || ts == TransferSyntax::JPEGLSLossless ||
        ts == TransferSyntax::JPEGLSNearLossless || ts == TransferSyntax::JPEG2000Lossless ||
        ts == TransferSyntax::JPEG2000 || ts == TransferSyntax::JPIPReferenced)
    {
      mdcmWarningMacro("Cant have Planar Configuration in JPEG/JPEG-LS/JPEG 2000. Discarding");
      PlanarConfiguration = 0;
    }
  }
  assert(PlanarConfiguration == 0 || PlanarConfiguration == 1);
}

void
Bitmap::Clear()
{
  Dimensions.clear();
}

bool
Bitmap::IsEmpty() const
{
  return (Dimensions.size() == 0);
}

const PhotometricInterpretation &
Bitmap::GetPhotometricInterpretation() const
{
  return PI;
}

void
Bitmap::SetPhotometricInterpretation(PhotometricInterpretation const & pi)
{
  PI = pi;
}

bool
Bitmap::IsLossy() const
{
  return LossyFlag;
}

void
Bitmap::SetLossyFlag(bool f)
{
  LossyFlag = f;
}

// For palette color multiply this length by 3,
// if computing the size of equivalent RGB image.
unsigned long long
Bitmap::GetBufferLength() const
{
  if (PF == PixelFormat::UNKNOWN)
    return 0;
  assert(NumberOfDimensions);
  if (NumberOfDimensions != Dimensions.size())
  {
    assert(Dimensions[2] == 1);
  }
  unsigned long long                        len = 0;
  unsigned long long                        mul = 1;
  std::vector<unsigned int>::const_iterator it = Dimensions.cbegin();
  for (; it != Dimensions.cend(); ++it)
  {
    mul *= *it;
  }
  if (PF == PixelFormat::UINT12 || PF == PixelFormat::INT12)
  {
    mul *= PF.GetPixelSize();
  }
  else if (PF == PixelFormat::SINGLEBIT)
  {
    assert(PF.GetSamplesPerPixel() == 1);
    const unsigned long long bytesPerRow =
      ((unsigned long long)Dimensions[0] / 8) + (((unsigned long long)Dimensions[0] % 8 != 0) ? 1 : 0);
    unsigned long long save = bytesPerRow * Dimensions[1];
    if (NumberOfDimensions > 2)
      save *= Dimensions[2];
    mul = save;
  }
  else if (PF.GetBitsAllocated() % 8 != 0)
  {
    assert(PF.GetSamplesPerPixel() == 1);
    const ByteValue * bv = PixelData.GetByteValue();
    if (bv)
    {
      unsigned long long ref = bv->GetLength() / mul;
      if (!GetTransferSyntax().IsEncapsulated())
      {
        mdcmAlwaysWarnMacro("GetBufferLength(): bv->GetLength()%mul != 0");
      }
      mul *= ref;
    }
  }
  else
  {
    mul *= PF.GetPixelSize();
  }
  len = mul;
  if (len == 0)
  {
    mdcmAlwaysWarnMacro("GetBufferLength() = 0");
  }
  return len;
}

bool
Bitmap::GetBuffer(char * buffer) const
{
  bool dummy;
  return GetBufferInternal(buffer, dummy);
}

bool
Bitmap::AreOverlaysInPixelData() const
{
  return false;
}

bool
Bitmap::UnusedBitsPresentInPixelData() const
{
  return false;
}

// Call SetPixelFormat first, before SetPlanarConfiguration!
bool
Bitmap::GetNeedByteSwap() const
{
  return NeedByteSwap;
}

void
Bitmap::SetNeedByteSwap(bool b)
{
  NeedByteSwap = b;
}

void
Bitmap::SetTransferSyntax(TransferSyntax const & ts)
{
  TS = ts;
}

const TransferSyntax &
Bitmap::GetTransferSyntax() const
{
  return TS;
}

bool
Bitmap::IsTransferSyntaxCompatible(TransferSyntax const & ts) const
{
  if (GetTransferSyntax() == ts)
    return true;
  if (GetTransferSyntax() == TransferSyntax::JPEGExtendedProcess2_4)
  {
    if (GetPixelFormat().GetBitsAllocated() == 8)
    {
      if (ts == TransferSyntax::JPEGBaselineProcess1)
        return true;
    }
  }
  return false;
}

void
Bitmap::SetDataElement(DataElement const & de)
{
  PixelData = de;
}

const DataElement &
Bitmap::GetDataElement() const
{
  return PixelData;
}

DataElement &
Bitmap::GetDataElement()
{
  return PixelData;
}

void
Bitmap::SetLUT(LookupTable const & lut)
{
  LUT = SmartPointer<LookupTable>(const_cast<LookupTable *>(&lut));
}

const LookupTable &
Bitmap::GetLUT() const
{
  return *LUT;
}

LookupTable &
Bitmap::GetLUT()
{
  return *LUT;
}

void
Bitmap::SetColumns(unsigned int col)
{
  SetDimension(0, col);
}

unsigned int
Bitmap::GetColumns() const
{
  return GetDimension(0);
}

void
Bitmap::SetRows(unsigned int rows)
{
  SetDimension(1, rows);
}

unsigned int
Bitmap::GetRows() const
{
  return GetDimension(1);
}

const PixelFormat &
Bitmap::GetPixelFormat() const
{
  return PF;
}

PixelFormat &
Bitmap::GetPixelFormat()
{
  return PF;
}

void
Bitmap::SetPixelFormat(PixelFormat const & pf)
{
  PF = pf;
  PF.Validate();
}

void
Bitmap::Print(std::ostream & os) const
{
  Object::Print(os);
  if (!IsEmpty())
  {
    os << "NumberOfDimensions: " << NumberOfDimensions << "\n";
    assert(Dimensions.size());
    os << "Dimensions: (";
    std::vector<unsigned int>::const_iterator it = Dimensions.cbegin();
    os << *it;
    for (; it != Dimensions.cend(); ++it)
    {
      os << "," << *it;
    }
    os << ")\n";
    PF.Print(os);
    os << "PhotometricInterpretation: " << PI << "\n";
    os << "PlanarConfiguration: " << PlanarConfiguration << "\n";
    os << "TransferSyntax: " << TS << "\n";
  }
}

// Image can be lossy but in implicit little endian format
bool
Bitmap::ComputeLossyFlag()
{
  bool lossyflag;
  if (this->GetBufferInternal(NULL, lossyflag))
  {
    LossyFlag = lossyflag;
    return true;
  }
  LossyFlag = false;
  return false;
}

bool
Bitmap::TryRAWCodec(char * buffer, bool & lossyflag) const
{
  RAWCodec               codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      if (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL_422)
        lossyflag = true;
      else
        lossyflag = false;
      return true;
    }
    return false;
  }
  const ByteValue * bv = PixelData.GetByteValue();
  if (bv)
  {
    unsigned long long len = GetBufferLength();
    if (!codec.CanDecode(ts))
      return false;
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetLUT(GetLUT());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNeedByteSwap(GetNeedByteSwap());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    DataElement out;
    const bool  r = codec.DecodeBytes(bv->GetPointer(), bv->GetLength(), buffer, len);
    if (!r)
      return false;
    if (GetNeedByteSwap())
    {
      // Internally DecodeBytes always does the byte-swapping step,
      // so remove internal flag
      Bitmap * i = const_cast<Bitmap *>(this);
      i->SetNeedByteSwap(false);
    }
    if (len != bv->GetLength())
    {
      mdcmDebugMacro("Pixel Length " << bv->GetLength() << " is different from computed value " << len);
    }
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEGCodec(char * buffer, bool & lossyflag) const
{
  JPEGCodec              codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
      {
        mdcmAlwaysWarnMacro("JPEG: SequenceOfFragments is NULL");
        return false;
      }
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      PixelFormat       pf = GetPixelFormat();
      codec.SetPixelFormat(pf);
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      if (!codec.GetHeaderInfo(ss, ts2))
      {
        mdcmAlwaysWarnMacro("JPEG: !codec.GetHeaderInfo(ss, ts2)");
        return false;
      }
      lossyflag = codec.IsLossy();
#if 0
      if (codec.GetPixelFormat() != GetPixelFormat())
      {
        Bitmap *i = (Bitmap*)this;
        i->SetPixelFormat(codec.GetPixelFormat());
      }
#else
      const PixelFormat & cpf = codec.GetPixelFormat();
      // SC16BitsAllocated_8BitsStoredJPEG.dcm
      if (cpf.GetBitsAllocated() <= pf.GetBitsAllocated())
      {
        if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
        {
          if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
          {
            if (cpf.GetBitsStored() < pf.GetBitsStored())
            {
              mdcmAlwaysWarnMacro("Encapsulated stream has fewer bits stored, fixed");
              Bitmap * i = const_cast<Bitmap *>(this);
              i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
#endif
      if (GetDimensions()[0] != codec.GetDimensions()[0] || GetDimensions()[1] != codec.GetDimensions()[1])
      {
        // JPEGNote_bogus.dcm
        mdcmAlwaysWarnMacro("JPEG: dimension mismatch for JPEG");
        (const_cast<Bitmap *>(this))->SetDimensions(codec.GetDimensions());
      }
      return true;
    }
    return false;
  }
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryJPEGCodec: value too big for ByteValue" << len);
    return false;
  }
  if (codec.CanDecode(ts))
  {
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetDimensions(GetDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    DataElement out;
    if (!codec.Decode(PixelData, out))
    {
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      mdcmAlwaysWarnMacro("JPEG: !codec.Decode(PixelData, out)");
      return false;
    }
    if (GetPlanarConfiguration() != codec.GetPlanarConfiguration())
    {
      Bitmap * i = const_cast<Bitmap *>(this);
      (void)i;
    }
#if 1
    const PixelFormat & cpf = codec.GetPixelFormat();
    const PixelFormat & pf = GetPixelFormat();
    if (pf != cpf)
    {
      // DCMTK_JPEGExt_12Bits.dcm
      if (pf.GetPixelRepresentation() == cpf.GetPixelRepresentation())
      {
        if (pf.GetBitsAllocated() == 12)
        {
          Bitmap * i = const_cast<Bitmap *>(this);
          i->GetPixelFormat().SetBitsAllocated(16);
          i->GetPixelFormat().SetBitsStored(12);
        }
      }
    }
#else
    const PixelFormat & cpf = codec.GetPixelFormat();
    const PixelFormat & pf = GetPixelFormat();
    // SC16BitsAllocated_8BitsStoredJPEG.dcm
    if (cpf.GetBitsAllocated() <= pf.GetBitsAllocated())
    {
      if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
      {
        if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
        {
          if (cpf.GetBitsStored() < pf.GetBitsStored())
          {
            mdcmAlwaysWarnMacro("Encapsulated stream has fewer bits stored, fixed");
            Bitmap * i = const_cast<Bitmap *>(this);
            i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
            i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
          }
        }
      }
    }
#endif
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
    {
      mdcmAlwaysWarnMacro("JPEG: !out.GetByteValue()");
      return false;
    }
    if (len != (unsigned long long)outbv->GetLength())
    {
      mdcmAlwaysWarnMacro("JPEG: length is " << len << ", should be " << outbv->GetLength());
    }
    if (buffer)
    {
      memcpy(buffer,
             outbv->GetPointer(),
             (len >= (unsigned long long)outbv->GetLength() ? (size_t)outbv->GetLength() : (size_t)len));
    }
    lossyflag = codec.IsLossy();
    return true;
  }
  return false;
}

bool
Bitmap::TryJPEGCodec2(std::ostream & os) const
{
  const TransferSyntax & ts = GetTransferSyntax();
  JPEGCodec              codec;
  if (codec.CanCode(ts))
  {
    codec.SetDimensions(GetDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    DataElement out;
    const bool  r = codec.Code(PixelData, out);
    if (!r)
      return false;
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
      return false;
    os.write(outbv->GetPointer(), outbv->GetLength());
    return true;
  }
  return false;
}

bool
Bitmap::TryJPEGCodec3(char * buffer, bool & lossyflag) const
{
  JPEGCodec              codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
      {
        mdcmAlwaysWarnMacro("JPEG: SequenceOfFragments is NULL");
        return false;
      }
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      PixelFormat       pf = GetPixelFormat();
      codec.SetPixelFormat(pf);
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      if (!codec.GetHeaderInfo(ss, ts2))
      {
        mdcmAlwaysWarnMacro("JPEG: !codec.GetHeaderInfo(ss, ts2)");
        return false;
      }
      lossyflag = codec.IsLossy();
#if 0
      if (codec.GetPixelFormat() != GetPixelFormat())
      {
        Bitmap *i = (Bitmap*)this;
        i->SetPixelFormat(codec.GetPixelFormat());
      }
#else
      const PixelFormat & cpf = codec.GetPixelFormat();
      // SC16BitsAllocated_8BitsStoredJPEG.dcm
      if (cpf.GetBitsAllocated() <= pf.GetBitsAllocated())
      {
        if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
        {
          if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
          {
            if (cpf.GetBitsStored() < pf.GetBitsStored())
            {
              mdcmAlwaysWarnMacro("Encapsulated stream has fewer bits stored, fixed");
              Bitmap * i = const_cast<Bitmap *>(this);
              i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
#endif
      if (GetDimensions()[0] != codec.GetDimensions()[0] || GetDimensions()[1] != codec.GetDimensions()[1])
      {
        // JPEGNote_bogus.dcm
        mdcmAlwaysWarnMacro("JPEG: dimension mismatch for JPEG");
        (const_cast<Bitmap *>(this))->SetDimensions(codec.GetDimensions());
      }
      return true;
    }
    return false;
  }
  if (codec.CanDecode(ts))
  {
    const unsigned long long len = GetBufferLength();
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetDimensions(GetDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    std::stringstream os;
    if (!codec.Decode2(PixelData, os))
    {
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      mdcmAlwaysWarnMacro("JPEG: !codec.Decode2(PixelData, os)");
      return false;
    }
    if (GetPlanarConfiguration() != codec.GetPlanarConfiguration())
    {
      Bitmap * i = const_cast<Bitmap *>(this);
      (void)i;
    }
    const PixelFormat & cpf = codec.GetPixelFormat();
    const PixelFormat & pf = GetPixelFormat();
    if (pf != cpf)
    {
      // DCMTK_JPEGExt_12Bits.dcm
      if (pf.GetPixelRepresentation() == cpf.GetPixelRepresentation())
      {
        if (pf.GetBitsAllocated() == 12)
        {
          Bitmap * i = const_cast<Bitmap *>(this);
          i->GetPixelFormat().SetBitsAllocated(16);
          i->GetPixelFormat().SetBitsStored(12);
        }
      }
    }
    const size_t len2 = os.tellp();
    if (len != len2)
    {
      mdcmAlwaysWarnMacro("TryJPEGCodec3: len=" << len << " len2=" << len2);
      if (len > len2)
      {
        memset(buffer, 0, len);
      }
    }
    os.seekp(0, std::ios::beg);
#if 1
    std::stringbuf * pbuf = os.rdbuf();
    pbuf->sgetn(buffer, ((len < len2) ? len : len2));
#else
    const std::string & tmp0 = os.str();
    const char * tmp1 = tmp0.data();
    memcpy(
      buffer,
      tmp1,
      ((len < len2) ? len : len2));
#endif
    lossyflag = codec.IsLossy();
    return true;
  }
  return false;
}

bool
Bitmap::TryPVRGCodec(char * buffer, bool & lossyflag) const
{
  if (!buffer) return false;
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryPVRGCodec: value too big for ByteValue" << len);
    return false;
  }
  const TransferSyntax & ts = GetTransferSyntax();
  PVRGCodec              codec;
  if (codec.CanDecode(ts))
  {
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetDimensions(GetDimensions());
    DataElement out;
    bool        r = codec.Decode(PixelData, out);
    if (!r)
      return false;
    codec.SetLossyFlag(ts.IsLossy());
    assert(r);
    if (GetPlanarConfiguration() != codec.GetPlanarConfiguration())
    {
      Bitmap * i = const_cast<Bitmap *>(this);
      i->PlanarConfiguration = codec.GetPlanarConfiguration();
    }
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
      return false;
    assert(len <= outbv->GetLength());
    if (buffer)
      memcpy(buffer, outbv->GetPointer(), (size_t)len);
    lossyflag = codec.IsLossy();
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEGLSCodec(char * buffer, bool & lossyflag) const
{
  JPEGLSCodec            codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      const bool b = codec.GetHeaderInfo(ss, ts2);
      if (!b)
        return false;
      lossyflag = codec.IsLossy();
      return true;
    }
    return false;
  }
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryJPEGLSCodec: value too big for ByteValue" << len);
    return false;
  }
  if (codec.CanDecode(ts))
  {
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetBufferLength(len);
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetDimensions(GetDimensions());
    DataElement out;
    const bool  r = codec.Decode(PixelData, out);
    if (!r)
      return false;
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
      return false;
    assert(len <= outbv->GetLength());
    if (buffer)
      memcpy(buffer, outbv->GetPointer(), (size_t)len);
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() != ts.IsLossy())
    {
      mdcmErrorMacro("declared as lossless but is in fact lossy.");
    }
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEGLSCodec2(char * buffer, bool & lossyflag) const
{
  JPEGLSCodec            codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      const bool b = codec.GetHeaderInfo(ss, ts2);
      if (!b)
        return false;
      lossyflag = codec.IsLossy();
      return true;
    }
    return false;
  }
  if (codec.CanDecode(ts))
  {
    const unsigned long long len = GetBufferLength();
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetBufferLength(len);
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetDimensions(GetDimensions());
    const bool r = codec.Decode2(PixelData, buffer, len);
    if (!r)
      return false;
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() != ts.IsLossy())
    {
      mdcmErrorMacro("declared as lossless but is in fact lossy.");
    }
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEG2000Codec(char * buffer, bool & lossyflag) const
{
  JPEG2000Codec          codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      const bool        b = codec.GetHeaderInfo(bv2.GetPointer(), bv2.GetLength(), ts2);
      if (!b)
        return false;
      lossyflag = codec.IsLossy();
      const PixelFormat & cpf = codec.GetPixelFormat();
      const PixelFormat & pf = GetPixelFormat();
      if (cpf.GetBitsAllocated() == pf.GetBitsAllocated())
      {
        if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
        {
          if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
          {
            if (cpf.GetBitsStored() < pf.GetBitsStored())
            {
              Bitmap * i = const_cast<Bitmap *>(this);
              mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk. correcting.");
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
      else
      {
        mdcmWarningMacro("Bits Allocated are different. This is pretty bad using info from codestream");
        Bitmap * i = const_cast<Bitmap *>(this);
        i->SetPixelFormat(codec.GetPixelFormat());
      }
      return true;
    }
    return false;
  }
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryJPEG2000Codec: value too big for ByteValue" << len);
    return false;
  }
  if (codec.CanDecode(ts))
  {
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetDimensions(GetDimensions());
    DataElement out;
    bool        r = codec.Decode(PixelData, out);
    if (!r)
      return false;
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
      return false;
    assert(len <= outbv->GetLength());
    if (buffer)
      memcpy(buffer, outbv->GetPointer(), (size_t)len);
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() && !ts.IsLossy())
    {
      assert(codec.IsLossy());
      assert(!ts.IsLossy());
      mdcmErrorMacro("declared as lossless but is in fact lossy.");
    }
    const PixelFormat & cpf = codec.GetPixelFormat();
    const PixelFormat & pf = GetPixelFormat();
    if (cpf.GetBitsAllocated() == pf.GetBitsAllocated())
    {
      if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
      {
        if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
        {
          if (cpf.GetBitsStored() < pf.GetBitsStored())
          {
            Bitmap * i = const_cast<Bitmap *>(this);
            mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk. correcting.");
            i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
          }
        }
      }
    }
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEG2000Codec2(std::ostream & os) const
{
  const TransferSyntax & ts = GetTransferSyntax();
  JPEG2000Codec          codec;
  if (codec.CanCode(ts))
  {
    codec.SetDimensions(GetDimensions());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    DataElement out;
    const bool  r = codec.Code(PixelData, out);
    if (!r)
      return false;
    const ByteValue * outbv = out.GetByteValue();
    if (!outbv)
      return false;
    os.write(outbv->GetPointer(), outbv->GetLength());
    return r;
  }
  return false;
}

bool
Bitmap::TryJPEG2000Codec3(char * buffer, bool & lossyflag) const
{
  JPEG2000Codec          codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      TransferSyntax              ts2;
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      const bool        b = codec.GetHeaderInfo(bv2.GetPointer(), bv2.GetLength(), ts2);
      if (!b)
        return false;
      lossyflag = codec.IsLossy();
      const PixelFormat & cpf = codec.GetPixelFormat();
      const PixelFormat & pf = GetPixelFormat();
      if (cpf.GetBitsAllocated() == pf.GetBitsAllocated())
      {
        if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
        {
          if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
          {
            if (cpf.GetBitsStored() < pf.GetBitsStored())
            {
              Bitmap * i = const_cast<Bitmap *>(this);
              mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk. correcting.");
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
      else
      {
        mdcmWarningMacro("Bits Allocated are different. This is pretty bad using info from codestream");
        Bitmap * i = const_cast<Bitmap *>(this);
        i->SetPixelFormat(codec.GetPixelFormat());
      }
      return true;
    }
    return false;
  }
  if (codec.CanDecode(ts))
  {
    const unsigned long long len = GetBufferLength();
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetDimensions(GetDimensions());
    const bool r = codec.Decode2(PixelData, buffer, len);
    if (!r)
      return false;
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() && !ts.IsLossy())
    {
      assert(codec.IsLossy());
      assert(!ts.IsLossy());
      mdcmErrorMacro("declared as lossless but is in fact lossy.");
    }
    const PixelFormat & cpf = codec.GetPixelFormat();
    const PixelFormat & pf = GetPixelFormat();
    if (cpf.GetBitsAllocated() == pf.GetBitsAllocated())
    {
      if (cpf.GetPixelRepresentation() == pf.GetPixelRepresentation())
      {
        if (cpf.GetSamplesPerPixel() == pf.GetSamplesPerPixel())
        {
          if (cpf.GetBitsStored() < pf.GetBitsStored())
          {
            Bitmap * i = const_cast<Bitmap *>(this);
            mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk. correcting.");
            i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
          }
        }
      }
    }
    return r;
  }
  return false;
}

bool
Bitmap::TryRLECodec(char * buffer, bool & lossyflag) const
{
  if (!buffer) return false;
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryRLECodec: value too big for ByteValue" << len);
    return false;
  }
  const TransferSyntax & ts = GetTransferSyntax();
  RLECodec               codec;
  if (codec.CanDecode(ts))
  {
    codec.SetDimensions(GetDimensions());
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetLUT(GetLUT());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    codec.SetBufferLength(len);
    DataElement out;
    const bool  r = codec.Decode(PixelData, out);
    if (!r)
      return false;
    const ByteValue * outbv = out.GetByteValue();
    assert(len <= outbv->GetLength());
    if (buffer)
      memcpy(buffer, outbv->GetPointer(), (size_t)len);
    lossyflag = false;
    return true;
  }
  return false;
}

#define DATA_MORE_THAN_4GB
bool
Bitmap::GetBufferInternal(char * buffer, bool & lossyflag) const
{
  bool success = TryRAWCodec(buffer, lossyflag);
  if (!success)
#ifdef DATA_MORE_THAN_4GB
    success = TryJPEGCodec3(buffer, lossyflag);
#else
    success = TryJPEGCodec(buffer, lossyflag);
#endif
  if (!success)
    success = TryPVRGCodec(buffer, lossyflag);
  if (!success)
#ifdef DATA_MORE_THAN_4GB
    success = TryJPEG2000Codec(buffer, lossyflag);
#else
    success = TryJPEG2000Codec3(buffer, lossyflag);
#endif
  if (!success)
#ifdef DATA_MORE_THAN_4GB
    success = TryJPEGLSCodec2(buffer, lossyflag);
#else
    success = TryJPEGLSCodec(buffer, lossyflag);
#endif
  if (!success)
    success = TryRLECodec(buffer, lossyflag);
  return success;
}
#ifdef DATA_MORE_THAN_4GB
#undef DATA_MORE_THAN_4GB
#endif

} // namespace mdcm
