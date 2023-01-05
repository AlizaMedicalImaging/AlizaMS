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
#include "mdcmJPEGLSCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmDataElement.h"
#include "mdcmSwapper.h"
#include <numeric>
#include <cstring>
#include "mdcm_charls.h"

#if defined(__GNUC__) && GCC_VERSION < 50101
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace mdcm
{

JPEGLSCodec::JPEGLSCodec()
  : BufferLength(0)
  , LossyError(0)
{}

JPEGLSCodec::~JPEGLSCodec() {}

bool
JPEGLSCodec::CanDecode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::JPEGLSLossless || ts == TransferSyntax::JPEGLSNearLossless);
}

bool
JPEGLSCodec::CanCode(TransferSyntax const & ts) const
{
  return ts == TransferSyntax::JPEGLSLossless || ts == TransferSyntax::JPEGLSNearLossless;
}

bool
JPEGLSCodec::Decode(DataElement const & in, DataElement & out)
{
  using namespace charls;
  if (NumberOfDimensions == 2)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    const size_t totalLen = sf->ComputeByteLength();
    char *       buffer;
    try
    {
      buffer = new char[totalLen];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    sf->GetBuffer(buffer, totalLen);
    std::vector<unsigned char> rgbyteOut;
    const bool                 b = DecodeByStreamsCommon(buffer, totalLen, rgbyteOut);
    if (!b)
      return false;
    delete[] buffer;
    out = in;
    out.SetByteValue(reinterpret_cast<char *>(&rgbyteOut[0]), static_cast<uint32_t>(rgbyteOut.size()));
    return true;
  }
  else if (NumberOfDimensions == 3)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    if (sf->GetNumberOfFragments() != Dimensions[2])
      return false;
    std::stringstream os;
    for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      const Fragment & frag = sf->GetFragment(i);
      if (frag.IsEmpty())
        return false;
      const ByteValue * bv = frag.GetByteValue();
      if (!bv)
        return false;
      size_t totalLen = bv->GetLength();
      char * mybuffer;
      try
      {
        mybuffer = new char[totalLen];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      bv->GetBuffer(mybuffer, bv->GetLength());
      const unsigned char * pbyteCompressed = reinterpret_cast<const unsigned char *>(mybuffer);
      while (totalLen > 0 && pbyteCompressed[totalLen - 1] != 0xd9)
      {
        totalLen--;
      }
      // what if 0xd9 is never found ?
      assert(totalLen > 0 && pbyteCompressed[totalLen - 1] == 0xd9);
      size_t        cbyteCompressed = totalLen;
      JlsParameters params = {};
      if (JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params, NULL) != ApiResult::OK)
      {
        mdcmDebugMacro("Could not parse JPEG-LS header");
        return false;
      }
      // allowedlossyerror == 0 => Lossless
      LossyFlag = params.allowedLossyError != 0;
      std::vector<unsigned char> rgbyteOut;
      rgbyteOut.resize(static_cast<size_t>(params.height) * static_cast<size_t>(params.width) * ((params.bitsPerSample + 7) / 8) * params.components);
      ApiResult result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), pbyteCompressed, cbyteCompressed, &params, NULL);
      delete[] mybuffer;
      if (result != ApiResult::OK)
      {
        return false;
      }
      os.write(reinterpret_cast<const char *>(&rgbyteOut[0]), rgbyteOut.size());
    }
    std::string str = os.str();
    assert(str.size());
    const unsigned long long str_size = str.size();
    if (str_size >= 0xffffffff)
    {
      mdcmAlwaysWarnMacro("JPEGLSCodec: value too big for ByteValue");
      return false;
    }
    out.SetByteValue(&str[0], static_cast<uint32_t>(str_size));
    return true;
  }
  return false;
}

bool
JPEGLSCodec::Decode2(DataElement const & in, char * out_buffer, size_t len)
{
  using namespace charls;
  if (NumberOfDimensions == 2)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    const size_t totalLen = sf->ComputeByteLength();
    char *       buffer;
    try
    {
      buffer = new char[totalLen];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    sf->GetBuffer(buffer, totalLen);
    const bool b = DecodeByStreamsCommon2(buffer, totalLen, out_buffer, len);
    if (!b)
      return false;
    delete[] buffer;
    return true;
  }
  else if (NumberOfDimensions == 3)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    if (sf->GetNumberOfFragments() != Dimensions[2])
      return false;
    std::stringstream os;
    for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      const Fragment & frag = sf->GetFragment(i);
      if (frag.IsEmpty())
        return false;
      const ByteValue * bv = frag.GetByteValue();
      if (!bv)
        return false;
      size_t totalLen = bv->GetLength();
      char * mybuffer;
      try
      {
        mybuffer = new char[totalLen];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      bv->GetBuffer(mybuffer, bv->GetLength());
      const unsigned char * pbyteCompressed = reinterpret_cast<const unsigned char *>(mybuffer);
      while (totalLen > 0 && pbyteCompressed[totalLen - 1] != 0xd9)
      {
        totalLen--;
      }
      // what if 0xd9 is never found ?
      assert(totalLen > 0 && pbyteCompressed[totalLen - 1] == 0xd9);
      size_t        cbyteCompressed = totalLen;
      JlsParameters params = {};
      if (JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params, NULL) != ApiResult::OK)
      {
        mdcmDebugMacro("Could not parse JPEG-LS header");
        return false;
      }
      // allowedlossyerror == 0 => Lossless
      LossyFlag = params.allowedLossyError != 0;
      const size_t tmp0_size =
        static_cast<size_t>(params.height) * static_cast<size_t>(params.width) * ((params.bitsPerSample + 7) / 8) * params.components;
      char * tmp0;
      try
      {
        tmp0 = new char[tmp0_size];
      }
      catch (const std::bad_alloc &)
      {
        delete[] mybuffer;
        return false;
      }
      ApiResult result = JpegLsDecode(tmp0, tmp0_size, pbyteCompressed, cbyteCompressed, &params, NULL);
      delete[] mybuffer;
      if (result != ApiResult::OK)
      {
        return false;
      }
      os.write(tmp0, tmp0_size);
      delete[] tmp0;
    }
    const size_t len2 = os.tellp();
    if (len2 != len)
    {
      mdcmAlwaysWarnMacro("JPEGLSCodec::Decode2: len=" << len << " len2=" << len2);
      if (len > len2)
      {
        memset(out_buffer, 0, len);
      }
    }
    os.seekp(0, std::ios::beg);
#if 1
    std::stringbuf * pbuf = os.rdbuf();
    pbuf->sgetn(out_buffer, ((len < len2) ? len : len2));
#else
    const std::string & tmp0 = os.str();
    const char * tmp1 = tmp0.data();
    memcpy(
      out_buffer,
      tmp1,
      ((len < len2) ? len : len2));
#endif
    return true;
  }
  return false;
}

bool
JPEGLSCodec::Code(DataElement const & in, DataElement & out)
{
  out = in;
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const unsigned int *              dims = this->GetDimensions();
  const int                         image_width = dims[0];
  const int                         image_height = dims[1];
  const ByteValue *                 bv = in.GetByteValue();
  const char *                      input = bv->GetPointer();
  const size_t                      len = bv->GetLength();
  const size_t                      image_len = len / dims[2];
  const size_t                      inputlength = image_len;
  for (unsigned int dim = 0; dim < dims[2]; ++dim)
  {
    const char *               inputdata = input + dim * image_len;
    std::vector<unsigned char> rgbyteCompressed;
    rgbyteCompressed.resize(static_cast<size_t>(image_width) * static_cast<size_t>(image_height) * 4 * 2); // overallocate
    size_t     cbyteCompressed;
    const bool b = this->CodeFrameIntoBuffer(
      reinterpret_cast<char *>(&rgbyteCompressed[0]), rgbyteCompressed.size(), cbyteCompressed, inputdata, inputlength);
    if (!b)
      return false;
    Fragment frag;
    frag.SetByteValue(reinterpret_cast<char *>(&rgbyteCompressed[0]), static_cast<uint32_t>(cbyteCompressed));
    sq->AddFragment(frag);
  }
  assert(sq->GetNumberOfFragments() == dims[2]);
  out.SetValue(*sq);
  return true;
}

unsigned long long
JPEGLSCodec::GetBufferLength() const
{
  return BufferLength;
}

void
JPEGLSCodec::SetBufferLength(unsigned long long l)
{
  BufferLength = l;
}

bool
JPEGLSCodec::GetHeaderInfo(std::istream & is, TransferSyntax & ts)
{
  using namespace charls;
  is.seekg(0, std::ios::end);
  const size_t buf_size = is.tellg();
  char *       dummy_buffer;
  try
  {
    dummy_buffer = new char[buf_size];
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
  is.seekg(0, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  JlsParameters metadata = {};
  if (JpegLsReadHeader(dummy_buffer, buf_size, &metadata, NULL) != ApiResult::OK)
  {
    return false;
  }
  delete[] dummy_buffer;
  this->Dimensions[0] = metadata.width;
  this->Dimensions[1] = metadata.height;
  if (metadata.bitsPerSample <= 8)
  {
    this->PF = PixelFormat(PixelFormat::UINT8);
  }
  else if (metadata.bitsPerSample <= 16)
  {
    this->PF = PixelFormat(PixelFormat::UINT16);
  }
  else
  {
    assert(0);
    return false;
  }
  this->PF.SetBitsStored(static_cast<uint16_t>(metadata.bitsPerSample));
  assert(this->PF.IsValid());
  if (metadata.components == 1)
  {
    PI = PhotometricInterpretation::MONOCHROME2;
    this->PF.SetSamplesPerPixel(1);
  }
  else if (metadata.components == 3)
  {
    PI = PhotometricInterpretation::RGB;
    this->PF.SetSamplesPerPixel(3);
  }
  else
  {
    assert(0);
  }
  // allowedlossyerror == 0 => Lossless
  LossyFlag = metadata.allowedLossyError != 0;
  if (metadata.allowedLossyError == 0)
  {
    ts = TransferSyntax::JPEGLSLossless;
  }
  else
  {
    ts = TransferSyntax::JPEGLSNearLossless;
  }
  return true;
}

void
JPEGLSCodec::SetLossless(bool l)
{
  LossyFlag = !l;
}

bool
JPEGLSCodec::GetLossless() const
{
  return !LossyFlag;
}

void
JPEGLSCodec::SetLossyError(int error)
{
  LossyError = error;
}

bool
JPEGLSCodec::DecodeExtent(char *         buffer,
                          unsigned int   xmin,
                          unsigned int   xmax,
                          unsigned int   ymin,
                          unsigned int   ymax,
                          unsigned int   zmin,
                          unsigned int   zmax,
                          std::istream & is)
{
  BasicOffsetTable bot;
  bot.Read<SwapperNoOp>(is);
  const unsigned int * dimensions = this->GetDimensions();
  const PixelFormat &  pf = this->GetPixelFormat();
  assert(pf.GetBitsAllocated() % 8 == 0);
  assert(pf != PixelFormat::SINGLEBIT);
  assert(pf != PixelFormat::UINT12 && pf != PixelFormat::INT12);
  if (NumberOfDimensions == 2)
  {
    char *            dummy_buffer = NULL;
    std::vector<char> vdummybuffer;
    size_t            buf_size = 0;
    const Tag         seqDelItem(0xfffe, 0xe0dd);
    Fragment          frag;
    while (frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
    {
      const size_t fraglen = frag.GetVL();
      const size_t oldlen = vdummybuffer.size();
      buf_size = fraglen + oldlen;
      vdummybuffer.resize(buf_size);
      dummy_buffer = &vdummybuffer[0];
      is.read(&vdummybuffer[oldlen], fraglen);
    }
    assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    assert(zmin == zmax);
    assert(zmin == 0);
    std::vector<unsigned char> outv;
    const bool                 b = DecodeByStreamsCommon(dummy_buffer, buf_size, outv);
    if (!b)
      return false;
    unsigned char *    raw = &outv[0];
    const size_t       rowsize = static_cast<size_t>(xmax) - static_cast<size_t>(xmin) + 1;
    const size_t       colsize = static_cast<size_t>(ymax) - static_cast<size_t>(ymin) + 1;
    const unsigned int bytesPerPixel = pf.GetPixelSize();
    if (outv.size() != dimensions[0] * dimensions[1] * bytesPerPixel)
    {
      mdcmDebugMacro("Inconsistant buffer size. Giving up");
      return false;
    }
    const unsigned char * tmpBuffer1 = raw;
    unsigned int          z = 0;
    for (unsigned int y = ymin; y <= ymax; ++y)
    {
      size_t theOffset =
        0 + (z * static_cast<size_t>(dimensions[1]) * static_cast<size_t>(dimensions[0]) + y * static_cast<size_t>(dimensions[0]) + xmin) * bytesPerPixel;
      tmpBuffer1 = raw + theOffset;
      memcpy(&(buffer[((z - zmin) * rowsize * colsize + (y - ymin) * rowsize) * bytesPerPixel]),
             tmpBuffer1,
             rowsize * bytesPerPixel);
    }
  }
  else if (NumberOfDimensions == 3)
  {
    const Tag           seqDelItem(0xfffe, 0xe0dd);
    Fragment            frag;
    std::streamoff      thestart = is.tellg();
    size_t              numfrags = 0;
    std::vector<size_t> offsets;
    while (frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
    {
#if 0
      std::streamoff relstart = is.tellg();
      assert(relstart - thestart == 8);
#endif
      std::streamoff off = frag.GetVL();
      offsets.push_back(static_cast<size_t>(off));
      is.seekg(off, std::ios::cur);
      ++numfrags;
    }
    assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    assert(numfrags == offsets.size());
    if (numfrags != Dimensions[2])
    {
      mdcmErrorMacro("Not handled");
      return false;
    }
    for (unsigned int z = zmin; z <= zmax; ++z)
    {
      const size_t curoffset = std::accumulate(offsets.begin(), offsets.begin() + z, 0);
      is.seekg(thestart + curoffset + 8 * z, std::ios::beg);
      is.seekg(8, std::ios::cur);
      const size_t buf_size = offsets[z];
      char *       dummy_buffer;
      try
      {
        dummy_buffer = new char[buf_size];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      is.read(dummy_buffer, buf_size);
      std::vector<unsigned char> outv;
      const bool                 b = DecodeByStreamsCommon(dummy_buffer, buf_size, outv);
      delete[] dummy_buffer;
      if (!b)
        return false;
      unsigned char *    raw = &outv[0];
      const size_t       rowsize = static_cast<size_t>(xmax) - static_cast<size_t>(xmin) + 1;
      const size_t       colsize = static_cast<size_t>(ymax) - static_cast<size_t>(ymin) + 1;
      const unsigned int bytesPerPixel = pf.GetPixelSize();
      if (outv.size() != dimensions[0] * dimensions[1] * bytesPerPixel)
      {
        mdcmDebugMacro("Inconsistant buffer size. Giving up");
        return false;
      }
      const unsigned char * tmpBuffer1 = raw;
      for (unsigned int y = ymin; y <= ymax; ++y)
      {
        const size_t theOffset =
          0 + (0 *  static_cast<size_t>(dimensions[1]) *  static_cast<size_t>(dimensions[0]) + y *  static_cast<size_t>(dimensions[0]) + xmin) * bytesPerPixel; //
        tmpBuffer1 = raw + theOffset;
        memcpy(&(buffer[((z - zmin) * rowsize * colsize + (y - ymin) * rowsize) * bytesPerPixel]),
               tmpBuffer1,
               rowsize * bytesPerPixel);
      }
    }
  }
  return true;
}

bool
JPEGLSCodec::StartEncode(std::ostream &)
{
  return true;
}

bool
JPEGLSCodec::IsRowEncoder()
{
  return false;
}

bool
JPEGLSCodec::IsFrameEncoder()
{
  return true;
}

bool
JPEGLSCodec::AppendRowEncode(std::ostream &, const char *, size_t)
{
  return false;
}

bool
JPEGLSCodec::AppendFrameEncode(std::ostream & out, const char * data, size_t datalen)
{
  const unsigned int * dimensions = this->GetDimensions();
  const PixelFormat &  pf = this->GetPixelFormat();
  (void)pf;
  assert(datalen == dimensions[0] * dimensions[1] * pf.GetPixelSize());
  std::vector<unsigned char> rgbyteCompressed;
  rgbyteCompressed.resize( static_cast<size_t>(dimensions[0]) *  static_cast<size_t>(dimensions[1]) * 4);
  size_t     cbyteCompressed;
  const bool b =
    this->CodeFrameIntoBuffer(reinterpret_cast<char *>(&rgbyteCompressed[0]), rgbyteCompressed.size(), cbyteCompressed, data, datalen);
  if (!b)
    return false;
  out.write(reinterpret_cast<char *>(&rgbyteCompressed[0]), cbyteCompressed);
  return true;
}

bool
JPEGLSCodec::StopEncode(std::ostream &)
{
  return true;
}

bool
JPEGLSCodec::DecodeByStreamsCommon(const char * buffer, size_t totalLen, std::vector<unsigned char> & rgbyteOut)
{
  using namespace charls;
  const unsigned char * pbyteCompressed = reinterpret_cast<const unsigned char *>(buffer);
  const size_t          cbyteCompressed = totalLen;
  JlsParameters         params = {};
  if (JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params, NULL) != ApiResult::OK)
  {
    mdcmDebugMacro("Could not parse JPEG-LS header");
    return false;
  }
  if (params.colorTransformation != ColorTransformation::None)
  {
    mdcmDebugMacro("JPEGLSCodec::DecodeByStreamsCommon: found color transformation " << static_cast<int>(params.colorTransformation));
  }
  // allowedlossyerror == 0 => Lossless
  LossyFlag = params.allowedLossyError != 0;
  rgbyteOut.resize(static_cast<size_t>(params.height) * static_cast<size_t>(params.width) * ((params.bitsPerSample + 7) / 8) * params.components);
  ApiResult result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), pbyteCompressed, cbyteCompressed, &params, NULL);
  if (result != ApiResult::OK)
  {
    mdcmErrorMacro("Could not decode JPEG-LS stream");
    return false;
  }
  return true;
}

bool
JPEGLSCodec::DecodeByStreamsCommon2(const char * buffer, size_t totalLen, char * out, size_t out_size)
{
  using namespace charls;
  const unsigned char * pbyteCompressed = reinterpret_cast<const unsigned char *>(buffer);
  const size_t          cbyteCompressed = totalLen;
  JlsParameters         params = {};
  if (JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params, NULL) != ApiResult::OK)
  {
    mdcmDebugMacro("Could not parse JPEG-LS header");
    return false;
  }
  if (params.colorTransformation != ColorTransformation::None)
  {
    mdcmDebugMacro("JPEGLSCodec::DecodeByStreamsCommon2: found color transformation " << (int)params.colorTransformation);
  }
  // allowedlossyerror == 0 => Lossless
  LossyFlag = params.allowedLossyError != 0;
  const size_t out_size2 =
    static_cast<size_t>(params.height) * static_cast<size_t>(params.width) * ((params.bitsPerSample + 7) / 8) * params.components;
  if (out_size != out_size2)
  {
    mdcmAlwaysWarnMacro("DecodeByStreamsCommon2: out_size=" << out_size << " out_size2=" << out_size2);
  }
  ApiResult result = JpegLsDecode(
    out, ((out_size > out_size2) ? out_size : out_size2), pbyteCompressed, cbyteCompressed, &params, NULL);
  if (result != ApiResult::OK)
  {
    mdcmErrorMacro("Could not decode JPEG-LS stream");
    return false;
  }
  return true;
}

bool
JPEGLSCodec::CodeFrameIntoBuffer(char * outdata, size_t outlen, size_t & complen, const char * indata, size_t inlen)
{
  using namespace charls;
  const unsigned int * dims = this->GetDimensions();
  const int            image_width = dims[0];
  const int            image_height = dims[1];
  const PixelFormat &  pf = this->GetPixelFormat();
  const int            samples_pixel = pf.GetSamplesPerPixel();
  const int            bitsallocated = pf.GetBitsAllocated();
  const int            bitsstored = pf.GetBitsStored();
  JlsParameters        params = {};
  /*
  The fields in JlsCustomParameters do not control lossy/lossless. They
  provide the possiblity to tune the JPEG-LS internals for better compression
  ratios. Expect a lot of work and testing to achieve small improvements.

  Lossy/lossless is controlled by the field allowedlossyError. If you put in
  0, encoding is lossless. If it is non-zero, then encoding is lossy. The
  value of 3 is often suggested as a default.
  */
  params.allowedLossyError = !LossyFlag ? 0 : LossyError;
  params.components = samples_pixel;
  params.bitsPerSample = bitsallocated;
  params.height = image_height;
  params.width = image_width;
  if (samples_pixel == 1)
  {
    params.interleaveMode = InterleaveMode::None;
  }
  else if (samples_pixel == 3)
  {
    params.interleaveMode = InterleaveMode::Sample;
    params.colorTransformation = ColorTransformation::None;
  }
  else
  {
    mdcmAlwaysWarnMacro("Not supported: samples per pixel " << samples_pixel);
    return false;
  }
  ApiResult error = JpegLsEncode(outdata, outlen, &complen, indata, inlen, &params, NULL);
  if (error != ApiResult::OK)
  {
    mdcmErrorMacro("Error compressing: " << static_cast<int>(error));
    return false;
  }
  assert(complen < outlen);
  return true;
}

} // end namespace mdcm
