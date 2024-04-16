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
#include "mdcmEncapsulatedRAWCodec.h"
#include "mdcmJPEGCodec.h"
#include "mdcmJPEGLSCodec.h"
#include "mdcmJPEG2000Codec.h"
#include "mdcmRLECodec.h"
#include "mdcmImageHelper.h"
#include <cstring>

#define MDCM_DATA_MORE_THAN_4GB
#define MDCM_TRYJPEG_USE_RDBUF

namespace mdcm
{

unsigned int
Bitmap::GetNumberOfDimensions() const
{
  return NumberOfDimensions;
}

void
Bitmap::SetNumberOfDimensions(unsigned int dim)
{
  if (!(dim == 2 || dim == 3))
  {
    mdcmAlwaysWarnMacro("Internal error, NumberOfDimensions = " << dim);
    NumberOfDimensions = 2;
  }
  NumberOfDimensions = dim;
  if (NumberOfDimensions != 3u)
  {
    Dimensions[2] = 1u;
  }
}

const unsigned int *
Bitmap::GetDimensions() const
{
  return Dimensions.data();
}

unsigned int
Bitmap::GetDimension(unsigned int idx) const
{
  if (idx < 3)
    return Dimensions[idx];
  return 0u;
}

void
Bitmap::SetDimensions(const unsigned int dims[3])
{
  Dimensions[0] = dims[0];
  Dimensions[1] = dims[1];
  if (NumberOfDimensions == 3)
    Dimensions[2] = dims[2];
  else
    Dimensions[2] = 1u;
}

void
Bitmap::SetDimension(unsigned int idx, unsigned int dim)
{
  if (idx < 3)
    Dimensions[idx] = dim;
}

unsigned int
Bitmap::GetPlanarConfiguration() const
{
  if (PlanarConfiguration != 0 && PF.GetSamplesPerPixel() != 3)
  {
    assert(0);
    mdcmWarningMacro("Can't set PlanarConfiguration if SamplesPerPixel is not 3");
    return 0u;
  }
  return PlanarConfiguration;
}

void
Bitmap::SetPlanarConfiguration(unsigned int pc)
{
  assert(pc == 0 || pc == 1);
  PlanarConfiguration = pc;
  if (pc != 0)
  {
    if (PF.GetSamplesPerPixel() != 3)
    {
      mdcmWarningMacro("Can not set Planar Configuration for the scalar image, discarding");
      PlanarConfiguration = 0;
    }
    const TransferSyntax & ts = GetTransferSyntax();
    if (ts == TransferSyntax::JPEGBaselineProcess1 ||
        ts == TransferSyntax::JPEGExtendedProcess2_4 ||
        ts == TransferSyntax::JPEGExtendedProcess3_5 ||
        ts == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
        ts == TransferSyntax::JPEGFullProgressionProcess10_12 ||
        ts == TransferSyntax::JPEGLosslessProcess14 ||
        ts == TransferSyntax::JPEGLosslessProcess14_1 ||
        ts == TransferSyntax::JPEGLSLossless ||
        ts == TransferSyntax::JPEGLSNearLossless ||
        ts == TransferSyntax::JPEG2000Lossless ||
        ts == TransferSyntax::JPEG2000 ||
        ts == TransferSyntax::HTJ2K ||
        ts == TransferSyntax::HTJ2KLossless ||
        ts == TransferSyntax::HTJ2KLosslessRPCL ||
        ts == TransferSyntax::JPIPReferenced ||
        ts == TransferSyntax::JPIPHTJ2KReferenced ||
        ts == TransferSyntax::JPIPHTJ2KReferencedDeflate)
    {
      mdcmWarningMacro("Can not have Planar Configuration for the transfer syntax "
                       << ts << ", discarding");
      PlanarConfiguration = 0;
    }
  }
}

void
Bitmap::ClearDimensions()
{
  Dimensions[0] = 0;
  Dimensions[1] = 0;
  Dimensions[2] = 1;
}

bool
Bitmap::IsDimensionEmpty() const
{
  if (Dimensions.at(0) == 0 || Dimensions.at(1) == 0)
    return true;
  return false;
}

const PhotometricInterpretation &
Bitmap::GetPhotometricInterpretation() const
{
  return PI;
}

void
Bitmap::SetPhotometricInterpretation(const PhotometricInterpretation & pi)
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
  {
    mdcmAlwaysWarnMacro("Unknown Pixel Format");
    return 0ull;
  }
  {
    if (NumberOfDimensions == 2 && Dimensions[2] != 1)
    {
      mdcmAlwaysWarnMacro("NumberOfDimensions is 2, but 3rd dimension is " << Dimensions[2]);
    }
    else if (NumberOfDimensions == 3 && Dimensions[2] == 0)
    {
      mdcmAlwaysWarnMacro("NumberOfDimensions is 3, but Dimensions[2] is 0");
      return 0ull;
    }
  }
  unsigned long long tmp0 = 1ull;
  if (PF == PixelFormat::SINGLEBIT)
  {
    if (PF.GetSamplesPerPixel() != 1u)
    {
      mdcmAlwaysWarnMacro("SINGLEBIT and SamplesPerPixel " << PF.GetSamplesPerPixel());
      return 0ull;
    }
#if 1
    unsigned long long size_bits = static_cast<unsigned long long>(Dimensions[0]) * Dimensions[1];
    if (GetTransferSyntax().IsEncapsulated())
    {
      if (size_bits % 8 != 0)
      {
        mdcmAlwaysWarnMacro(
          "Currently not supported: SINGLEBIT, encapsulated transfer syntax and "
          << Dimensions[0] << " x " << Dimensions[1]);
        return 0ull;
      }
    }
    if (NumberOfDimensions > 2u)
    {
      size_bits *= Dimensions[2];
    }
    if (size_bits % 8 != 0)
    {
      tmp0 = size_bits / 8ull + 1ull;
    }
    else
    {
      tmp0 = size_bits / 8ull;
    }
#else
    // Wrong, but may be used for some broken data?
    unsigned long long size_bits = Dimensions[0] / 8ull + (Dimensions[0] % 8 != 0 ? 1 : 0);
    size_bits *= Dimensions[1];
    if (NumberOfDimensions > 2u)
    {
      size_bits *= Dimensions[2];
    }
    tmp0 = size_bits;
#endif
  }
  else
  {
    tmp0 *= Dimensions[0];
    tmp0 *= Dimensions[1];
    if (NumberOfDimensions > 2u)
    {
      tmp0 *= Dimensions[2];
    }
    const unsigned short bits_allocated = PF.GetBitsAllocated();
    if (bits_allocated % 8 != 0)
    {
      mdcmAlwaysWarnMacro("Bits Allocated " << bits_allocated << " is invalid");
      if (PF == PixelFormat::UINT12 || PF == PixelFormat::INT12)
      {
        tmp0 *= 2ull;
      }
      else
      {
        return 0ull;
      }
    }
    else
    {
      tmp0 *= PF.GetPixelSize();
    }
  }
  return tmp0;
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
Bitmap::SetTransferSyntax(const TransferSyntax & ts)
{
  TS = ts;
}

const TransferSyntax &
Bitmap::GetTransferSyntax() const
{
  return TS;
}

bool
Bitmap::IsTransferSyntaxCompatible(const TransferSyntax & ts) const
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
Bitmap::SetDataElement(const DataElement & de)
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
Bitmap::SetLUT(const LookupTable & lut)
{
  LUT = lut;
}

const LookupTable &
Bitmap::GetLUT() const
{
  return LUT;
}

LookupTable &
Bitmap::GetLUT()
{
  return LUT;
}

void
Bitmap::SetColumns(unsigned int col)
{
  Dimensions[0] = col;
}

unsigned int
Bitmap::GetColumns() const
{
  return Dimensions[0];
}

void
Bitmap::SetRows(unsigned int rows)
{
  Dimensions[1] = rows;
}

unsigned int
Bitmap::GetRows() const
{
  return Dimensions[1];
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
Bitmap::SetPixelFormat(const PixelFormat & pf)
{
  PF = pf;
  PF.Validate();
}

void
Bitmap::Print(std::ostream & os) const
{
  Object::Print(os);
  os << "NumberOfDimensions: " << NumberOfDimensions << "\nDimensions: ("
     << Dimensions[0] << ", " << Dimensions[1] << ", " << Dimensions[2] << ")\n";
  PF.Print(os);
  os << "\nPhotometricInterpretation: " << PI
     << "\nPlanarConfiguration: " << PlanarConfiguration
     << "\nTransferSyntax: " << TS << '\n';
}

// Image can be lossy but in implicit little endian format
bool
Bitmap::ComputeLossyFlag()
{
  bool lossyflag;
  if (this->GetBufferInternal(nullptr, lossyflag))
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
Bitmap::TryEncapsulatedRAWCodec(char * buffer, bool & lossyflag) const
{
  EncapsulatedRAWCodec   codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      lossyflag = false;
    }
    return false;
  }
  const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
  if (!sf)
  {
    mdcmAlwaysWarnMacro("EncapsulatedRAWCodec: SequenceOfFragments is null");
    return false;
  }
  if (codec.CanDecode(ts))
  {
    unsigned long long len = GetBufferLength();
    codec.SetNumberOfDimensions(GetNumberOfDimensions());
    codec.SetDimensions(GetDimensions());
    codec.SetPlanarConfiguration(GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(GetPhotometricInterpretation());
    codec.SetLUT(GetLUT());
    codec.SetPixelFormat(GetPixelFormat());
    codec.SetNeedByteSwap(GetNeedByteSwap());
    codec.SetNeedOverlayCleanup(AreOverlaysInPixelData() ||
                                (ImageHelper::GetCleanUnusedBits() && UnusedBitsPresentInPixelData()));
    DataElement out;
    const bool  r = codec.Decode2(PixelData, buffer, len);
    if (!r)
      return false;
    if (GetNeedByteSwap())
    {
      // TODO
      return false;
    }
    return true;
  }
  return false;
}

bool
Bitmap::TryJPEGCodec(char * buffer, bool & lossyflag) const
#ifdef MDCM_DATA_MORE_THAN_4GB
{
  JPEGCodec              codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
      {
        mdcmAlwaysWarnMacro("TryJPEGCodec: SequenceOfFragments is null");
        return false;
      }
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      PixelFormat       pf = GetPixelFormat();
      codec.SetPixelFormat(pf);
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      if (!codec.GetHeaderInfo(ss))
      {
        mdcmAlwaysWarnMacro("TryJPEGCodec: !codec.GetHeaderInfo(ss)");
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
            if (cpf.GetBitsStored() != pf.GetBitsStored())
            {
              mdcmAlwaysWarnMacro(
                "TryJPEGCodec:: encapsulated stream precision " << cpf.GetBitsStored() <<
                " bits, but DICOM bits stored " << pf.GetBitsStored());
              if (ImageHelper::GetFixJpegBits())
              {
                mdcmAlwaysWarnMacro("... fixed, assumed JPEG header is correct");
                Bitmap * i = const_cast<Bitmap *>(this);
                i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
                i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
              }
            }
          }
        }
      }
#endif
      if (Dimensions[0] != codec.GetDimensions()[0] || Dimensions[1] != codec.GetDimensions()[1])
      {
        // JPEGNote_bogus.dcm
        mdcmAlwaysWarnMacro("TryJPEGCodec: dimension mismatch for JPEG");
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
      mdcmAlwaysWarnMacro("TryJPEGCodec: !codec.Decode2(PixelData, os)");
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
      mdcmAlwaysWarnMacro("TryJPEGCodec: len=" << len << " len2=" << len2);
      if (len > len2)
      {
        memset(buffer, 0, len);
      }
    }
#ifdef MDCM_TRYJPEG_USE_RDBUF
    std::stringbuf * pbuf = os.rdbuf();
    const long long sgetn_s = pbuf->sgetn(buffer, ((len < len2) ? len : len2));
#if 0
    std::cout << "TryJPEGCodec: sizes should be the equal: " << len << " " << len2 << " " << sgetn_s << std::endl;
#endif
    if (sgetn_s <= 0)
    {
      mdcmAlwaysWarnMacro("TryJPEGCodec: pbuf->sgetn returned " << sgetn_s);
    }
#else
    const std::string & tmp0 = os.str();
    memcpy(
      buffer,
      tmp0.data(),
      ((len < len2) ? len : len2));
#endif
    lossyflag = codec.IsLossy();
    return true;
  }
  return false;
}
#else
{
  JPEGCodec              codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
      {
        mdcmAlwaysWarnMacro("JPEG: SequenceOfFragments is null");
        return false;
      }
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      PixelFormat       pf = GetPixelFormat();
      codec.SetPixelFormat(pf);
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      if (!codec.GetHeaderInfo(ss))
      {
        mdcmAlwaysWarnMacro("JPEG: !codec.GetHeaderInfo(ss)");
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
            if (cpf.GetBitsStored() != pf.GetBitsStored())
            {
              mdcmAlwaysWarnMacro(
                "Encapsulated stream reports precision " << cpf.GetBitsStored() <<
                " bits, but DICOM bits stored " << pf.GetBitsStored());
              if (ImageHelper::GetFixJpegBits())
              {
                mdcmAlwaysWarnMacro("... fixed, assumed JPEG header is correct");
                Bitmap * i = const_cast<Bitmap *>(this);
                i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
                i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
              }
            }
          }
        }
      }
#endif
      if (Dimensions[0] != codec.GetDimensions()[0] || Dimensions[1] != codec.GetDimensions()[1])
      {
        mdcmAlwaysWarnMacro("TryJPEGCodec: dimension mismatch");
        (const_cast<Bitmap *>(this))->SetDimensions(codec.GetDimensions());
      }
      return true;
    }
    return false;
  }
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryJPEGCodec: size " << len << " is not supported");
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
          if (cpf.GetBitsStored() != pf.GetBitsStored())
          {
            mdcmAlwaysWarnMacro(
              "Encapsulated stream reports precision " << cpf.GetBitsStored() <<
              " bits, but DICOM bits stored " << pf.GetBitsStored());
            if (ImageHelper::GetFixJpegBits())
            {
              mdcmAlwaysWarnMacro("... fixed, assumed JPEG header is correct");
              Bitmap * i = const_cast<Bitmap *>(this);
              i->GetPixelFormat().SetBitsAllocated(cpf.GetBitsAllocated());
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
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
    if (len != static_cast<unsigned long long>(outbv->GetLength()))
    {
      mdcmAlwaysWarnMacro("JPEG: length is " << len << ", should be " << outbv->GetLength());
    }
    memcpy(buffer,
           outbv->GetPointer(),
           (len >= static_cast<unsigned long long>(outbv->GetLength()) ? static_cast<size_t>(outbv->GetLength()) : static_cast<size_t>(len)));
    lossyflag = codec.IsLossy();
    return true;
  }
  return false;
}
#endif

bool
Bitmap::TryJPEGLSCodec(char * buffer, bool & lossyflag) const
#ifdef MDCM_DATA_MORE_THAN_4GB
{
  JPEGLSCodec            codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      const bool b = codec.GetHeaderInfo(ss);
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
      mdcmErrorMacro("Declared as lossless but is in fact lossy.");
    }
    return r;
  }
  return false;
}
#else
{
  JPEGLSCodec            codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      std::stringstream ss;
      ss.write(bv2.GetPointer(), bv2.GetLength());
      const bool b = codec.GetHeaderInfo(ss);
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
    mdcmAlwaysWarnMacro("TryJPEGLSCodec: size " << len << " is not supported");
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
    memcpy(buffer, outbv->GetPointer(), static_cast<size_t>(len));
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() != ts.IsLossy())
    {
      mdcmErrorMacro("Declared as lossless but is in fact lossy.");
    }
    return r;
  }
  return false;
}
#endif


bool
Bitmap::TryJPEG2000Codec(char * buffer, bool & lossyflag) const
#ifdef MDCM_DATA_MORE_THAN_4GB
{
  JPEG2000Codec          codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      const bool        b = codec.GetHeaderInfo(bv2.GetPointer(), bv2.GetLength());
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
              mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk, correcting.");
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
      else
      {
        mdcmWarningMacro("Bits Allocated are different.");
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
      mdcmErrorMacro("Declared as lossless but is in fact lossy.");
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
            mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk, correcting.");
            i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
          }
        }
      }
    }
    return r;
  }
  return false;
}
#else
{
  JPEG2000Codec          codec;
  const TransferSyntax & ts = GetTransferSyntax();
  if (!buffer)
  {
    if (codec.CanDecode(ts))
    {
      const SequenceOfFragments * sf = PixelData.GetSequenceOfFragments();
      if (!sf)
        return false;
      const Fragment &  frag = sf->GetFragment(0);
      const ByteValue & bv2 = dynamic_cast<const ByteValue &>(frag.GetValue());
      const bool        b = codec.GetHeaderInfo(bv2.GetPointer(), bv2.GetLength());
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
              mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk, correcting.");
              i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
            }
          }
        }
      }
      else
      {
        mdcmWarningMacro("Bits Allocated are different.");
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
    mdcmAlwaysWarnMacro("TryJPEG2000Codec: size " << len << " is not supported");
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
    memcpy(buffer, outbv->GetPointer(), static_cast<size_t>(len));
    lossyflag = codec.IsLossy();
    if (codec.IsLossy() && !ts.IsLossy())
    {
      assert(codec.IsLossy());
      assert(!ts.IsLossy());
      mdcmErrorMacro("Declared as lossless but is in fact lossy.");
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
            mdcmWarningMacro("Encapsulated stream has fewer bits actually stored on disk, correcting.");
            i->GetPixelFormat().SetBitsStored(cpf.GetBitsStored());
          }
        }
      }
    }
    return r;
  }
  return false;
}
#endif

bool
Bitmap::TryRLECodec(char * buffer, bool & lossyflag) const
{
  if (!buffer) return false;
  const unsigned long long len = GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("TryRLECodec: size " << len << " is not supported");
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
    memcpy(buffer, outbv->GetPointer(), static_cast<size_t>(len));
    lossyflag = false;
    return true;
  }
  return false;
}

bool
Bitmap::GetBufferInternal(char * buffer, bool & lossyflag) const
{
  bool success = TryRAWCodec(buffer, lossyflag);
  if (!success)
    success = TryJPEGCodec(buffer, lossyflag);
  if (!success)
    success = TryJPEG2000Codec(buffer, lossyflag);
  if (!success)
    success = TryJPEGLSCodec(buffer, lossyflag);
  if (!success)
    success = TryRLECodec(buffer, lossyflag);
  if (!success)
    success = TryEncapsulatedRAWCodec(buffer, lossyflag);
  return success;
}

} // namespace mdcm
