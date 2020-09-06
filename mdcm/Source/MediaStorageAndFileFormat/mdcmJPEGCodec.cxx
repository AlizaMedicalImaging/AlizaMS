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

JPEGCodec::JPEGCodec():BitSample(0),Quality(100)
{
  Internal = NULL;
}

JPEGCodec::~JPEGCodec()
{
  if(Internal) delete Internal;
}

void JPEGCodec::SetQuality(double q)
{
  Quality = (int)q;
}

double JPEGCodec::GetQuality() const
{
  return Quality;
}

void JPEGCodec::SetLossless(bool l)
{
  LossyFlag = !l;
}

bool JPEGCodec::GetLossless() const
{
  return !LossyFlag;
}

bool JPEGCodec::CanDecode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEGBaselineProcess1
      || ts == TransferSyntax::JPEGExtendedProcess2_4
      || ts == TransferSyntax::JPEGExtendedProcess3_5
      || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8
      || ts == TransferSyntax::JPEGFullProgressionProcess10_12
      || ts == TransferSyntax::JPEGLosslessProcess14
      || ts == TransferSyntax::JPEGLosslessProcess14_1;
}

bool JPEGCodec::CanCode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEGBaselineProcess1
      || ts == TransferSyntax::JPEGExtendedProcess2_4
      || ts == TransferSyntax::JPEGExtendedProcess3_5
      || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8
      || ts == TransferSyntax::JPEGFullProgressionProcess10_12
      || ts == TransferSyntax::JPEGLosslessProcess14
      || ts == TransferSyntax::JPEGLosslessProcess14_1;
}

void JPEGCodec::SetPixelFormat(PixelFormat const &pt)
{
  ImageCodec::SetPixelFormat(pt);
  SetBitSample(pt.GetBitsAllocated());
}

void JPEGCodec::SetupJPEGBitCodec(int bit)
{
  BitSample = bit;
  delete Internal;
  Internal = NULL;
  // single bit images?
  if (BitSample <= 8)
  {
    mdcmDebugMacro("Using JPEG8");
    Internal = new JPEG8Codec;
  }
  else if (BitSample <= 12)
  {
    mdcmDebugMacro("Using JPEG12");
    Internal = new JPEG12Codec;
  }
  else if (BitSample <= 16)
  {
    mdcmDebugMacro("Using JPEG16");
    Internal = new JPEG16Codec;
  }
  else
  {
    // mdcmNonImageData/RT/RTDOSE.dcm
    mdcmAlwaysWarnMacro("JPEG: cannot instantiate codec for bit sample " << bit);
    delete Internal;
    Internal = NULL;
  }
}

void JPEGCodec::SetBitSample(int bit)
{
  SetupJPEGBitCodec(bit);
  if(Internal)
  {
    Internal->SetDimensions(this->GetDimensions());
    Internal->SetPlanarConfiguration(this->GetPlanarConfiguration());
    Internal->SetPhotometricInterpretation(this->GetPhotometricInterpretation());
    Internal->SetLossless(this->GetLossless());
    Internal->SetQuality(this->GetQuality());
    Internal->ImageCodec::SetPixelFormat(this->ImageCodec::GetPixelFormat());
    //Internal->SetNeedOverlayCleanup(this->AreOverlaysInPixelData());
  }
}

bool JPEGCodec::Decode(DataElement const &in, DataElement &out)
{
  assert(Internal);
  out = in;
  const SequenceOfFragments *sf0 = in.GetSequenceOfFragments();
  const ByteValue *jpegbv = in.GetByteValue();
  if(!sf0 && !jpegbv)
  {
    mdcmAlwaysWarnMacro("JPEG: !sf0 && !jpegbv");
    return false;
  }
  std::stringstream os;
  if(sf0)
  {
    for(unsigned int i = 0; i < sf0->GetNumberOfFragments(); ++i)
    {
      std::stringstream is;
      const Fragment &frag = sf0->GetFragment(i);
      if(frag.IsEmpty())
      {
        mdcmAlwaysWarnMacro("JPEG: frag.IsEmpty()");
        return false;
      }
      const ByteValue &bv = dynamic_cast<const ByteValue&>(frag.GetValue());
      size_t bv_len = bv.GetLength();
      char *mybuffer = new char[bv_len];
      bool b = bv.GetBuffer(mybuffer, bv.GetLength());
      assert(b); (void)b;
      is.write(mybuffer, bv.GetLength());
      delete[] mybuffer;
      bool r = DecodeByStreams(is, os);
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      if(!r)
      {
        mdcmDebugMacro("JPEG: failed to decompress Frag #" << i);
        const bool suspended = Internal->IsStateSuspension();
        const size_t nfrags = sf0->GetNumberOfFragments();
        // In case of chunked-jpeg, this is always an error
        if(suspended)
        {
          mdcmAlwaysWarnMacro("JPEG: suspended");
          return false;
        }
        // Ok so we are decoding a multiple frame jpeg DICOM file:
        // if we are lucky, we might be trying to decode some sort of broken multi-frame
        // DICOM file. In this case check that we have read all Fragment properly:
        if(i >= this->GetDimensions()[2])
        {
          // JPEGInvalidSecondFrag.dcm
          assert(nfrags == this->GetNumberOfDimensions()); (void)nfrags; // sentinel
          mdcmAlwaysWarnMacro("JPEG: invalid fragment found at #" << i + 1 << ", skipped");
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
    size_t jpegbv_len = jpegbv->GetLength();
    char *mybuffer0 = new char[jpegbv_len];
    bool b0 = jpegbv->GetBuffer(mybuffer0, jpegbv->GetLength());
    assert(b0); (void)b0;
    is0.write(mybuffer0, jpegbv->GetLength());
    delete[] mybuffer0;
    bool r = DecodeByStreams(is0, os);
    if(!r)
    {
      // let's try another time: JPEGDefinedLengthSequenceOfFragments.dcm
      is0.seekg(0);
      SequenceOfFragments sf_bug;
      try { sf_bug.Read<SwapperNoOp>(is0,true); }
      catch (...)
      {
        mdcmAlwaysWarnMacro("JPEG: unknown exception");
        return false;
      }
      const SequenceOfFragments *sf = &sf_bug;
      for(unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
      {
        std::stringstream is;
        const Fragment &frag = sf->GetFragment(i);
        if(frag.IsEmpty())
        {
          mdcmAlwaysWarnMacro("JPEG: frag.IsEmpty() (2)");
          return false;
        }
        const ByteValue &bv = dynamic_cast<const ByteValue&>(frag.GetValue());
        size_t bv_len = bv.GetLength();
        char *mybuffer = new char[bv_len];
        bool b = bv.GetBuffer(mybuffer, bv.GetLength());
        assert(b); (void)b;
        is.write(mybuffer, bv.GetLength());
        delete[] mybuffer;
        const bool r2 = DecodeByStreams(is, os);
        if(!r2)
        {
          mdcmAlwaysWarnMacro("JPEG: !r2");
          return false;
        }
      }
    }
  }
  const size_t sizeOfOs = (size_t)os.tellp();
  os.seekp(0, std::ios::beg);
  ByteValue * bv = new ByteValue;
  bv->SetLength((uint32_t)sizeOfOs);
  bv->Read<SwapperNoOp>(os);
  out.SetValue(*bv);
  return true;
}

void JPEGCodec::ComputeOffsetTable(bool)
{
  // Not implemented
}

bool JPEGCodec::GetHeaderInfo(std::istream & is, TransferSyntax &ts)
{
  assert(Internal);
  if (!Internal->GetHeaderInfo(is, ts))
  {
    // check if this is one of those buggy lossless JPEG
    if(this->BitSample != Internal->BitSample)
    {
      // MARCONI_MxTWin-12-MONO2-JpegLossless-ZeroLengthSQ.dcm
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      is.seekg(0, std::ios::beg);
      SetupJPEGBitCodec(Internal->BitSample);
      if(Internal && Internal->GetHeaderInfo(is, ts))
      {
        this->SetLossyFlag(Internal->GetLossyFlag());
        this->SetDimensions(Internal->GetDimensions());
        this->SetPhotometricInterpretation(Internal->GetPhotometricInterpretation());
        int prep = this->GetPixelFormat().GetPixelRepresentation();
        this->PF = Internal->GetPixelFormat(); // Do not call SetPixelFormat
        this->PF.SetPixelRepresentation((uint16_t)prep);
        return true;
      }
      else
      {
        mdcmAlwaysWarnMacro("JPEG: do not support this type");
        return false;
      }
    }
    return false;
  }
  // Foward everything back to meta jpeg codec:
  this->SetLossyFlag(Internal->GetLossyFlag());
  this->SetDimensions(Internal->GetDimensions());
  this->SetPhotometricInterpretation(Internal->GetPhotometricInterpretation());
  this->PF = Internal->GetPixelFormat(); // DO NOT CALL SetPixelFormat
  if(this->PI != Internal->PI)
  {
    mdcmWarningMacro("JPEG: possible Photometric Interpretation issue");
    this->PI = Internal->PI;
  }
  return true;
}

bool JPEGCodec::Code(DataElement const &in, DataElement &out)
{
  out = in;
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const Tag itemStart(0xfffe, 0xe000);
  const ByteValue *bv = in.GetByteValue();
  if(!bv) return false;
  const unsigned int *dims = this->GetDimensions();
  const char *input = bv->GetPointer();
  unsigned long len = bv->GetLength();
  unsigned long image_len = len / dims[2];
  if(!Internal) return false;
  Internal->SetLossless(this->GetLossless());
  Internal->SetQuality(this->GetQuality());
  for(unsigned int dim = 0; dim < dims[2]; ++dim)
  {
    std::stringstream os;
    const char *p = input + dim * image_len;
    bool r = Internal->InternalCode(p, image_len, os);
    if(!r) return false;
    std::string str = os.str();
    assert(str.size());
    Fragment frag;
    VL::Type strSize = (VL::Type)str.size();
    frag.SetByteValue(&str[0], strSize);
    sq->AddFragment(frag);
  }
  assert(sq->GetNumberOfFragments() == dims[2]);
  out.SetValue(*sq);
  return true;
}


bool JPEGCodec::DecodeByStreams(std::istream &is, std::ostream &os)
{
  std::stringstream tmpos;
  if (!Internal->DecodeByStreams(is,tmpos))
  {
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    // let's check if this is one of those buggy lossless JPEG
    if(this->BitSample != Internal->BitSample)
    {
      // MARCONI_MxTWin-12-MONO2-JpegLossless-ZeroLengthSQ.dcm
      // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
      if(this->BitSample < Internal->BitSample)
      {
        //assert(0); // Outside buffer will be too small
      }
      is.seekg(0, std::ios::beg);
      SetupJPEGBitCodec(Internal->BitSample);
      if(Internal)
      {
        Internal->SetDimensions(this->GetDimensions());
        Internal->SetPlanarConfiguration(this->GetPlanarConfiguration());
        Internal->SetPhotometricInterpretation(this->GetPhotometricInterpretation());
        if(Internal->DecodeByStreams(is,tmpos))
        {
          return ImageCodec::DecodeByStreams(tmpos,os);
        }
        else
        {
          mdcmErrorMacro("JPEG: SetupJPEGBitCodec failed");
        }
      }
    }
#endif
    mdcmAlwaysWarnMacro("JPEG: !Internal->DecodeByStreams(is,tmpos)");
    return false;
  }
  if(this->PlanarConfiguration != Internal->PlanarConfiguration)
  {
    mdcmAlwaysWarnMacro("JPEG: possible PlanarConfiguration issue");
    this->PlanarConfiguration = Internal->PlanarConfiguration;
  }
  if(this->PI != Internal->PI)
  {
    mdcmAlwaysWarnMacro("JPEG: possible PhotometricInterpretation issue");
    this->PI = Internal->PI;
  }
  if(this->PF == PixelFormat::UINT12 || this->PF == PixelFormat::INT12)
  {
    this->PF.SetBitsAllocated(16);
  }
  return ImageCodec::DecodeByStreams(tmpos,os);
}

bool JPEGCodec::IsValid(PhotometricInterpretation const &pi)
{
  // JPEGCodec can produce the following PhotometricInterpretation as output
  bool ret = false;
  switch(pi)
  {
    case PhotometricInterpretation::MONOCHROME1:
    case PhotometricInterpretation::MONOCHROME2:
    case PhotometricInterpretation::PALETTE_COLOR:
    case PhotometricInterpretation::RGB:
    case PhotometricInterpretation::YBR_FULL:
    case PhotometricInterpretation::YBR_FULL_422:
    case PhotometricInterpretation::YBR_PARTIAL_422: // FIXME
    case PhotometricInterpretation::YBR_PARTIAL_420: // FIXME
      ret = true;
      break;
    default:
      break;
  }
  return ret;
}

bool JPEGCodec::DecodeExtent(
    char *buffer,
    unsigned int xmin, unsigned int xmax,
    unsigned int ymin, unsigned int ymax,
    unsigned int zmin, unsigned int zmax,
    std::istream & is)
{
  BasicOffsetTable bot;
  bot.Read<SwapperNoOp>(is);
  const unsigned int * dimensions = this->GetDimensions();
  const PixelFormat & pf = this->GetPixelFormat();
  if (pf == PixelFormat::SINGLEBIT) 
  {
    mdcmAlwaysWarnMacro("JPEG: pf == PixelFormat::SINGLEBIT");
    return false;
  }
  if(NumberOfDimensions == 2)
  {
    std::vector<char> vdummybuffer;
    size_t buf_size = 0;

    const Tag seqDelItem(0xfffe,0xe0dd);
    Fragment frag;
    unsigned int nfrags = 0;
    try
    {
      while(frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
      {
        ++nfrags;
        size_t fraglen = frag.GetVL();
        size_t oldlen = vdummybuffer.size();
        buf_size = fraglen + oldlen;
        vdummybuffer.resize(buf_size);
        is.read(&vdummybuffer[oldlen], fraglen);
      }
      assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    }
    catch(Exception & ex)
    {
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
      // that's ok ! In all cases the whole file was read, because
      // Fragment::Read only fail on eof() reached 1.
      // SIEMENS-JPEG-CorruptFrag.dcm is more difficult to deal with, we have a
      // partial fragment, read we decide to add it anyway to the stack of
      // fragments (eof was reached so we need to clear error bit)
      if(frag.GetTag() == Tag(0xfffe,0xe000))
      {
        mdcmAlwaysWarnMacro("JPEG: Pixel Data Fragment could be corrupted");
        is.clear(); // clear the error bit
      }
      // 2. GENESIS_SIGNA-JPEG-CorruptFrag.dcm
      else if (frag.GetTag() == Tag(0xddff,0x00e0))
      {
        assert(nfrags == 1);
        const ByteValue *bv = frag.GetByteValue();
        assert((unsigned char)bv->GetPointer()[ bv->GetLength() - 1 ] == 0xfe);
        // Yes this is an extra copy, this is a bug anyway, go fix YOUR code
        frag.SetByteValue(bv->GetPointer(), bv->GetLength() - 1);
        assert(0);
        mdcmAlwaysWarnMacro("JPEG: fragment length was declared with an extra byte");
        is.clear(); // clear the error bit
      }
      else
      {
        // 3. mdcm-JPEG-LossLess3a.dcm: easy case, an extra tag was found
        // instead of terminator (eof is the next char)
        mdcmAlwaysWarnMacro("JPEG: reading failed at tag:" << frag.GetTag() << " " << ex.what());
      }
#endif
      mdcmAlwaysWarnMacro("JPEG: exception: " << ex.what());
    }
    assert(zmin == zmax);
    assert(zmin == 0);
    std::stringstream iis;
    iis.write(&vdummybuffer[0], vdummybuffer.size());
    std::stringstream os;
    if(!DecodeByStreams(iis,os))
    {
      mdcmAlwaysWarnMacro("JPEG: !DecodeByStreams(iis,os)");
      return false;
    }
    const unsigned int rowsize = xmax - xmin + 1;
    const unsigned int colsize = ymax - ymin + 1;
    const unsigned int bytesPerPixel = pf.GetPixelSize();
    os.seekg(0, std::ios::beg);
    assert(os.good());
    std::istream *theStream = &os;
    std::vector<char> buffer1;
    buffer1.resize(rowsize*bytesPerPixel);
    char *tmpBuffer1 = &buffer1[0];
    unsigned int y, z;
    std::streamoff theOffset;
    for (z = zmin; z <= zmax; ++z)
    {
      for (y = ymin; y <= ymax; ++y)
      {
        theStream->seekg(std::ios::beg);
        theOffset = 0 + (z*dimensions[1]*dimensions[0] + y*dimensions[0] + xmin)*bytesPerPixel;
        theStream->seekg(theOffset);
        theStream->read(tmpBuffer1, rowsize*bytesPerPixel);
        memcpy(
          &(buffer[((z-zmin)*rowsize*colsize + (y-ymin)*rowsize)*bytesPerPixel]),
          tmpBuffer1, rowsize*bytesPerPixel);
      }
    }
  }
  else if (NumberOfDimensions == 3)
  {
    const Tag seqDelItem(0xfffe,0xe0dd);
    Fragment frag;
    std::streamoff thestart = is.tellg();
    unsigned int numfrags = 0;
    std::vector< size_t > offsets;
    while(frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
    {
      const std::streamoff off = frag.GetVL();
      offsets.push_back((size_t)off);
      is.seekg(off, std::ios::cur);
      ++numfrags;
    }
    assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    assert(numfrags == offsets.size());
    if(numfrags != Dimensions[2])
    {
      mdcmAlwaysWarnMacro("JPEG: numfrags != Dimensions[2]");
      return false;
    }
    for(unsigned int z = zmin; z <= zmax; ++z)
    {
      size_t curoffset = std::accumulate(offsets.begin(), offsets.begin() + z, size_t(0));
      is.seekg(thestart + curoffset + 8 * z, std::ios::beg);
      is.seekg(8, std::ios::cur);
      std::stringstream os;
      const bool b = DecodeByStreams(is, os); (void)b;
      assert(b);
      os.seekg(0, std::ios::beg);
      assert(os.good());
      std::istream *theStream = &os;
      unsigned int rowsize = xmax - xmin + 1;
      unsigned int colsize = ymax - ymin + 1;
      unsigned int bytesPerPixel = pf.GetPixelSize();
      std::vector<char> buffer1;
      buffer1.resize(rowsize*bytesPerPixel);
      char *tmpBuffer1 = &buffer1[0];
      unsigned int y;
      std::streamoff theOffset;
      for (y = ymin; y <= ymax; ++y)
      {
        theStream->seekg(std::ios::beg);
        theOffset = 0 + (0*dimensions[1]*dimensions[0] + y*dimensions[0] + xmin)*bytesPerPixel;
        theStream->seekg(theOffset);
        theStream->read(tmpBuffer1, rowsize*bytesPerPixel);
        memcpy(
          &(buffer[((z-zmin)*rowsize*colsize + (y-ymin)*rowsize)*bytesPerPixel]),
          tmpBuffer1, rowsize*bytesPerPixel);
      }
    }
  }
  return true;
}

bool JPEGCodec::IsStateSuspension() const
{
  assert(0);
  return false;
}

ImageCodec * JPEGCodec::Clone() const
{
  JPEGCodec *copy = new JPEGCodec;
  ImageCodec &ic = *copy;
  ic = *this;
  assert(copy->PF == PF);
  copy->SetPixelFormat(GetPixelFormat());
  assert(copy->BitSample == BitSample || BitSample == 0);
  copy->Quality = Quality;
  return copy;
}

bool JPEGCodec::EncodeBuffer(std::ostream & out,
    const char *inbuffer, size_t inlen)
{
  assert(Internal);
  return Internal->EncodeBuffer(out, inbuffer, inlen);
}

bool JPEGCodec::StartEncode(std::ostream &)
{
  return true;
}
bool JPEGCodec::IsRowEncoder()
{
  return true;
}
bool JPEGCodec::IsFrameEncoder()
{
  assert(0);
  return false;
}
bool JPEGCodec::AppendRowEncode(std::ostream & os, const char * data, size_t datalen)
{
  return EncodeBuffer(os, data, datalen);
}
// TODO: technically the frame encoder could use the row encoder when present
// this could reduce code duplication
bool JPEGCodec::AppendFrameEncode(std::ostream & , const char * , size_t)
{
  assert(0);
  return false;
}
bool JPEGCodec::StopEncode(std::ostream &)
{
  return true;
}

} // end namespace mdcm
