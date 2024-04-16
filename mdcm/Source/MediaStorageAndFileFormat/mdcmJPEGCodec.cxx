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
#include "mdcmJPEGCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmTrace.h"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSwapper.h"
#include "mdcmJPEG8Codec.h"
#include "mdcmJPEG12Codec.h"
#include "mdcmJPEG16Codec.h"
#include <numeric>
#include <cstring>

namespace mdcm
{

JPEGCodec::JPEGCodec()
  : BitSample(0)
  , Quality(100)
{
  Internal = nullptr;
}

JPEGCodec::~JPEGCodec()
{
  delete Internal;
}

bool
JPEGCodec::CanDecode(const TransferSyntax & ts) const
{
  return ts == TransferSyntax::JPEGBaselineProcess1 || ts == TransferSyntax::JPEGExtendedProcess2_4 ||
         ts == TransferSyntax::JPEGExtendedProcess3_5 || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
         ts == TransferSyntax::JPEGFullProgressionProcess10_12 || ts == TransferSyntax::JPEGLosslessProcess14 ||
         ts == TransferSyntax::JPEGLosslessProcess14_1;
}

bool
JPEGCodec::CanCode(const TransferSyntax & ts) const
{
  return ts == TransferSyntax::JPEGBaselineProcess1 || ts == TransferSyntax::JPEGExtendedProcess2_4 ||
         ts == TransferSyntax::JPEGExtendedProcess3_5 || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
         ts == TransferSyntax::JPEGFullProgressionProcess10_12 || ts == TransferSyntax::JPEGLosslessProcess14 ||
         ts == TransferSyntax::JPEGLosslessProcess14_1;
}

bool
JPEGCodec::Decode(const DataElement & in, DataElement & out)
{
  assert(Internal);
  out = in;
  const SequenceOfFragments * sf0 = in.GetSequenceOfFragments();
  const ByteValue *           jpegbv = in.GetByteValue();
  if (!sf0 && !jpegbv)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: !sf0 && !jpegbv");
    return false;
  }
  std::stringstream os;
  if (sf0)
  {
    for (unsigned int i = 0; i < sf0->GetNumberOfFragments(); ++i)
    {
      std::stringstream is;
      const Fragment &  frag = sf0->GetFragment(i);
      if (frag.IsEmpty())
      {
        mdcmAlwaysWarnMacro("JPEGCodec: frag.IsEmpty()");
        return false;
      }
      const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
      const size_t      bv_len = bv.GetLength();
      char *            mybuffer;
      try
      {
        mybuffer = new char[bv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      const bool b = bv.GetBuffer(mybuffer, bv_len);
      if (!b)
      {
        delete[] mybuffer;
        return false;
      }
      is.write(mybuffer, bv_len);
      delete[] mybuffer;
      const bool r = DecodeByStreams(is, os);
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      if (!r)
      {
        mdcmDebugMacro("JPEGCodec: failed to decompress Frag #" << i);
        const bool   suspended = Internal->IsStateSuspension();
        const size_t nfrags = sf0->GetNumberOfFragments();
        // In case of chunked-jpeg, this is always an error
        if (suspended)
        {
          mdcmAlwaysWarnMacro("JPEGCodec: suspended");
          return false;
        }
        // Ok so we are decoding a multiple frame jpeg DICOM file:
        // if we are lucky, we might be trying to decode some sort of broken multi-frame
        // DICOM file. In this case check that we have read all Fragment properly:
        if (i >= this->GetDimensions()[2])
        {
          // JPEGInvalidSecondFrag.dcm
          assert(nfrags == this->GetNumberOfDimensions());
          (void)nfrags;
          mdcmAlwaysWarnMacro("JPEGCodec: invalid fragment found at #" << i + 1 << ", skipped");
        }
        else
        {
          return false;
        }
      }
    }
  }
  else if (jpegbv)
  {
    // GEIIS Icon
    std::stringstream is0;
    const size_t      jpegbv_len = jpegbv->GetLength();
    char *            mybuffer0;
    try
    {
      mybuffer0 = new char[jpegbv_len];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    const bool b0 = jpegbv->GetBuffer(mybuffer0, jpegbv_len);
    if (!b0)
    {
      delete[] mybuffer0;
      return false;
    }
    is0.write(mybuffer0, jpegbv_len);
    delete[] mybuffer0;
    const bool r = DecodeByStreams(is0, os);
    if (!r)
    {
      // Let's try another time: JPEGDefinedLengthSequenceOfFragments.dcm
      is0.seekg(0);
      SequenceOfFragments sf_bug;
      try
      {
        sf_bug.Read<SwapperNoOp>(is0, true);
      }
      catch (...)
      {
        mdcmAlwaysWarnMacro("JPEGCodec: unknown exception");
        return false;
      }
      const SequenceOfFragments * sf = &sf_bug;
      for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
      {
        std::stringstream is;
        const Fragment &  frag = sf->GetFragment(i);
        if (frag.IsEmpty())
        {
          mdcmAlwaysWarnMacro("JPEGCodec: frag.IsEmpty() (2)");
          return false;
        }
        const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
        const size_t      bv_len = bv.GetLength();
        char *            mybuffer;
        try
        {
          mybuffer = new char[bv_len];
        }
        catch (const std::bad_alloc &)
        {
          return false;
        }
        const bool b = bv.GetBuffer(mybuffer, bv_len);
        if (!b)
        {
          delete[] mybuffer;
          return false;
        }
        is.write(mybuffer, bv_len);
        delete[] mybuffer;
        const bool r2 = DecodeByStreams(is, os);
        if (!r2)
        {
          mdcmAlwaysWarnMacro("JPEGCodec: !r2");
          return false;
        }
      }
    }
  }
  const unsigned long long sizeOfOs = os.tellp();
  if (sizeOfOs >= 0xffffffff)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: value too big for ByteValue");
    return false;
  }
  os.seekp(0, std::ios::beg);
  ByteValue * bv = new ByteValue;
  bv->SetLength(static_cast<uint32_t>(sizeOfOs));
  bv->Read<SwapperNoOp>(os);
  out.SetValue(*bv);
  return true;
}

bool
JPEGCodec::Decode2(const DataElement & in, std::stringstream & os)
{
  assert(Internal);
  const SequenceOfFragments * sf0 = in.GetSequenceOfFragments();
  const ByteValue *           jpegbv = in.GetByteValue();
  if (!sf0 && !jpegbv)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: !sf0 && !jpegbv");
    return false;
  }
  if (sf0)
  {
    for (unsigned int i = 0; i < sf0->GetNumberOfFragments(); ++i)
    {
      std::stringstream is;
      const Fragment &  frag = sf0->GetFragment(i);
      if (frag.IsEmpty())
      {
        mdcmAlwaysWarnMacro("JPEGCodec: frag.IsEmpty()");
        return false;
      }
      const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
      const size_t      bv_len = bv.GetLength();
      char *            mybuffer;
      try
      {
        mybuffer = new char[bv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      const bool b = bv.GetBuffer(mybuffer, bv_len);
      if (!b)
      {
        delete[] mybuffer;
        return false;
      }
      is.write(mybuffer, bv_len);
      delete[] mybuffer;
      const bool r = DecodeByStreams(is, os);
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      if (!r)
      {
        mdcmDebugMacro("JPEGCodec: failed to decompress Frag #" << i);
        const bool   suspended = Internal->IsStateSuspension();
        const size_t nfrags = sf0->GetNumberOfFragments();
        // In case of chunked-jpeg, this is always an error
        if (suspended)
        {
          mdcmAlwaysWarnMacro("JPEGCodec: suspended");
          return false;
        }
        // Ok so we are decoding a multiple frame jpeg DICOM file:
        // if we are lucky, we might be trying to decode some sort of broken multi-frame
        // DICOM file. In this case check that we have read all Fragment properly:
        if (i >= this->GetDimensions()[2])
        {
          // JPEGInvalidSecondFrag.dcm
          assert(nfrags == this->GetNumberOfDimensions());
          (void)nfrags;
          mdcmAlwaysWarnMacro("JPEGCodec: invalid fragment found at #" << i + 1 << ", skipped");
        }
        else
        {
          return false;
        }
      }
    }
  }
  else if (jpegbv)
  {
    // GEIIS Icon
    std::stringstream is0;
    const size_t      jpegbv_len = jpegbv->GetLength();
    char *            mybuffer0;
    try
    {
      mybuffer0 = new char[jpegbv_len];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    const bool b0 = jpegbv->GetBuffer(mybuffer0, jpegbv_len);
    if (!b0)
    {
      delete[] mybuffer0;
      return false;
    }
    is0.write(mybuffer0, jpegbv_len);
    delete[] mybuffer0;
    const bool r = DecodeByStreams(is0, os);
    if (!r)
    {
      // Let's try another time: JPEGDefinedLengthSequenceOfFragments.dcm
      is0.seekg(0);
      SequenceOfFragments sf_bug;
      try
      {
        sf_bug.Read<SwapperNoOp>(is0, true);
      }
      catch (...)
      {
        mdcmAlwaysWarnMacro("JPEGCodec: unknown exception");
        return false;
      }
      const SequenceOfFragments * sf = &sf_bug;
      for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
      {
        std::stringstream is;
        const Fragment &  frag = sf->GetFragment(i);
        if (frag.IsEmpty())
        {
          mdcmAlwaysWarnMacro("JPEGCodec: frag.IsEmpty() (2)");
          return false;
        }
        const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
        const size_t      bv_len = bv.GetLength();
        char *            mybuffer;
        try
        {
          mybuffer = new char[bv_len];
        }
        catch (const std::bad_alloc &)
        {
          return false;
        }
        const bool b = bv.GetBuffer(mybuffer, bv_len);
        if (!b)
        {
          delete[] mybuffer;
          return false;
        }
        is.write(mybuffer, bv_len);
        delete[] mybuffer;
        const bool r2 = DecodeByStreams(is, os);
        if (!r2)
        {
          mdcmAlwaysWarnMacro("JPEGCodec: !r2");
          return false;
        }
      }
    }
  }
  return true;
}

bool
JPEGCodec::Code(const DataElement & in, DataElement & out)
{
  out = in;
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const ByteValue *                 bv = in.GetByteValue();
  if (!bv)
    return false;
  const unsigned int * dims = this->GetDimensions();
  const char *         input = bv->GetPointer();
  const size_t         len = bv->GetLength();
  const size_t         image_len = len / dims[2];
  if (!Internal)
    return false;
  Internal->SetLossless(this->GetLossless());
  Internal->SetQuality(this->GetQuality());
  for (unsigned int dim = 0; dim < dims[2]; ++dim)
  {
    std::stringstream os;
    const char *      p = input + dim * image_len;
    const bool        r = Internal->InternalCode(p, image_len, os);
    if (!r)
      return false;
    std::string str = os.str();
    assert(str.size());
    Fragment frag;
    VL::Type strSize = static_cast<VL::Type>(str.size());
    frag.SetByteValue(str.data(), strSize);
    sq->AddFragment(frag);
  }
  assert(sq->GetNumberOfFragments() == dims[2]);
  out.SetValue(*sq);
  return true;
}

void
JPEGCodec::SetPixelFormat(const PixelFormat & pf)
{
  ImageCodec::SetPixelFormat(pf);
  SetBitSample(pf.GetBitsStored()); //
}

void
JPEGCodec::ComputeOffsetTable(bool)
{
  // Not implemented
}

bool
JPEGCodec::GetHeaderInfo(std::istream & is)
{
  TransferSyntax ts;
  const bool r = GetHeaderInfoAndTS(is, ts);
  (void) ts;
  return r;
}

bool
JPEGCodec::GetHeaderInfoAndTS(std::istream & is, TransferSyntax & ts)
{
  if (!Internal) return false;
  if (!Internal->GetHeaderInfoAndTS(is, ts))
  {
    // check if this is one of those buggy lossless JPEG
    if (this->BitSample != Internal->BitSample)
    {
      // MARCONI_MxTWin-12-MONO2-JpegLossless-ZeroLengthSQ.dcm
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      is.seekg(0, std::ios::beg);
      SetupJPEGBitCodec(Internal->BitSample);
      if (Internal && Internal->GetHeaderInfoAndTS(is, ts))
      {
        this->SetLossyFlag(Internal->GetLossyFlag());
        this->SetDimensions(Internal->GetDimensions());
        this->SetPhotometricInterpretation(Internal->GetPhotometricInterpretation());
        const int prep = this->GetPixelFormat().GetPixelRepresentation();
        this->PF = Internal->GetPixelFormat(); // Do not call SetPixelFormat
        this->PF.SetPixelRepresentation(static_cast<uint16_t>(prep));
        return true;
      }
      else
      {
        mdcmAlwaysWarnMacro("JPEGCodec: not supported");
        return false;
      }
    }
    return false;
  }
  // Foward everything back to meta jpeg codec
  this->SetLossyFlag(Internal->GetLossyFlag());
  this->SetDimensions(Internal->GetDimensions());
  this->SetPhotometricInterpretation(Internal->GetPhotometricInterpretation());
  this->PF = Internal->GetPixelFormat(); // Do not call SetPixelFormat
  return true;
}

void
JPEGCodec::SetQuality(double q)
{
  Quality = static_cast<int>(q);
}

double
JPEGCodec::GetQuality() const
{
  return Quality;
}

void
JPEGCodec::SetLossless(bool b)
{
  LossyFlag = !b;
}

bool
JPEGCodec::GetLossless() const
{
  return !LossyFlag;
}

bool
JPEGCodec::EncodeBuffer(std::ostream & out, const char * inbuffer, size_t inlen)
{
  assert(Internal);
  return Internal->EncodeBuffer(out, inbuffer, inlen);
}

bool
JPEGCodec::StartEncode(std::ostream &)
{
  return true;
}

bool
JPEGCodec::StopEncode(std::ostream &)
{
  return true;
}

bool
JPEGCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  std::stringstream tmpos;
  if (!Internal->DecodeByStreams(is, tmpos))
  {
    if (this->BitSample != Internal->BitSample)
    {
      is.seekg(0, std::ios::beg);
      SetupJPEGBitCodec(Internal->BitSample);
      if (Internal)
      {
        Internal->SetDimensions(this->GetDimensions());
        Internal->SetPlanarConfiguration(this->GetPlanarConfiguration());
        Internal->SetPhotometricInterpretation(this->GetPhotometricInterpretation());
        if (Internal->DecodeByStreams(is, tmpos))
        {
          return ImageCodec::DecodeByStreams(tmpos, os);
        }
      }
      mdcmErrorMacro("JPEGCodec: SetupJPEGBitCodec failed");
    }
    mdcmAlwaysWarnMacro("JPEGCodec: DecodeByStreams() error");
    return false;
  }
  if (this->PlanarConfiguration != Internal->PlanarConfiguration)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: possible PlanarConfiguration issue");
    this->PlanarConfiguration = Internal->PlanarConfiguration;
  }
  if (this->PI != Internal->PI)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: possible PhotometricInterpretation issue");
    this->PI = Internal->PI;
  }
#if 1
  if (this->PF == PixelFormat::UINT12 || this->PF == PixelFormat::INT12)
  {
    mdcmAlwaysWarnMacro("JPEGCodec: PixelFormat is UINT12 or INT12");
  }
#endif
  return ImageCodec::DecodeByStreams(tmpos, os);
}

bool
JPEGCodec::InternalCode(const char *, size_t, std::ostream &)
{
  return false;
}

bool
JPEGCodec::IsValid(const PhotometricInterpretation & pi)
{
  switch (pi)
  {
    case PhotometricInterpretation::MONOCHROME1:
    case PhotometricInterpretation::MONOCHROME2:
    case PhotometricInterpretation::PALETTE_COLOR:
    case PhotometricInterpretation::RGB:
    case PhotometricInterpretation::YBR_FULL:
    case PhotometricInterpretation::YBR_FULL_422:
    case PhotometricInterpretation::YBR_PARTIAL_422: // Retired
      return true;
    case PhotometricInterpretation::YBR_PARTIAL_420: // MPEG
      mdcmAlwaysWarnMacro("YBR_PARTIAL_420 is not intended to be here");
      return true;
    default:
      break;
  }
  return false;
}

bool
JPEGCodec::IsRowEncoder()
{
  return true;
}

bool
JPEGCodec::IsFrameEncoder()
{
  assert(0);
  return false;
}

bool
JPEGCodec::AppendRowEncode(std::ostream & os, const char * data, size_t datalen)
{
  return EncodeBuffer(os, data, datalen);
}

bool
JPEGCodec::AppendFrameEncode(std::ostream &, const char *, size_t)
{
  // Technically the frame encoder could use the row encoder when present
  // this could reduce code duplication
  assert(0);
  return false;
}

void
JPEGCodec::SetBitSample(int bit)
{
  SetupJPEGBitCodec(bit);
  if (Internal)
  {
    Internal->SetDimensions(this->GetDimensions());
    Internal->SetPlanarConfiguration(this->GetPlanarConfiguration());
    Internal->SetPhotometricInterpretation(this->GetPhotometricInterpretation());
    Internal->SetLossless(this->GetLossless());
    Internal->SetQuality(this->GetQuality());
    Internal->ImageCodec::SetPixelFormat(this->ImageCodec::GetPixelFormat());
#if 0
    // TODO check this
    Internal->SetNeedOverlayCleanup(this->AreOverlaysInPixelData());
#endif
  }
}

bool
JPEGCodec::IsStateSuspension() const
{
  assert(0);
  return false;
}

void
JPEGCodec::SetupJPEGBitCodec(int b)
{
  BitSample = b;
  delete Internal;
  Internal = nullptr;
  if (BitSample <= 8)
  {
    Internal = new JPEG8Codec;
    mdcmDebugMacro("JPEGCodec: using JPEG8, bits=" << b);
  }
  else if (BitSample <= 12)
  {
    Internal = new JPEG12Codec;
    mdcmDebugMacro("JPEGCodec: using JPEG12, bits=" << b);
  }
  else if (BitSample <= 16)
  {
    Internal = new JPEG16Codec;
    mdcmDebugMacro("JPEGCodec: using JPEG16, bits=" << b);
  }
  else
  {
    mdcmAlwaysWarnMacro("JPEGCodec: SetupJPEGBitCodec(" << b << ") failed");
  }
}

} // end namespace mdcm
