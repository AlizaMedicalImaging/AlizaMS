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
#include "mdcmRAWCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmByteSwap.h"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include <limits>
#include <sstream>
#include <cstring>

namespace mdcm
{

// Unpack an array of 'packed' 12bits data into a more conventional 16bits
// array. n is the length in bytes of array in, out will be a 16bits
// array of size (n / 3) * 2
static bool
Unpack12Bits(char * out, const char * in, size_t n)
{
  // 3 bytes = 2 words
  // http://groups.google.com/group/comp.lang.c/msg/572bc9b085c717f3
  if (n % 3)
  {
    return false;
  }
  void *                vout = static_cast<void*>(out);
  const void *          vin = static_cast<const void*>(in);
  short *               q = static_cast<short *>(vout);
  const unsigned char * p = static_cast<const unsigned char *>(vin);
  const unsigned char * end = p + n;
  unsigned char         b0, b1, b2;
  while (p != end)
  {
    b0 = *p++;
    b1 = *p++;
    b2 = *p++;
    *q++ = static_cast<short>(((b1 & 0xf) << 8) + b0);
    *q++ = static_cast<short>((b1 >> 4) + (b2 << 4));
  }
  return true;
}

RAWCodec::RAWCodec() {}

RAWCodec::~RAWCodec() {}

bool
RAWCodec::CanCode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian || ts == TransferSyntax::ExplicitVRLittleEndian ||
          ts == TransferSyntax::ExplicitVRBigEndian || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE ||
          ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool
RAWCodec::CanDecode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian || ts == TransferSyntax::ExplicitVRLittleEndian ||
          ts == TransferSyntax::ExplicitVRBigEndian || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE ||
          ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool
RAWCodec::Code(DataElement const & in, DataElement & out)
{
  out = in;
  return true;
}

bool
RAWCodec::Decode(DataElement const & in, DataElement & out)
{
  if (!NeedByteSwap && !RequestPaddedCompositePixelCode && PI == PhotometricInterpretation::MONOCHROME2 &&
      !PlanarConfiguration && !RequestPlanarConfiguration && GetPixelFormat().GetBitsAllocated() != 12 &&
      !NeedOverlayCleanup)
  {
    out = in;
    return true;
  }
  const ByteValue * bv = in.GetByteValue();
  if (!bv)
  {
    return false;
  }
  std::stringstream is;
  is.write(bv->GetPointer(), bv->GetLength());
  std::stringstream os;
  const bool        r = DecodeByStreams(is, os);
  if (!r)
  {
    return false;
  }
  std::string str = os.str();
  out = in;
  if (this->GetPixelFormat() == PixelFormat::UINT12 || this->GetPixelFormat() == PixelFormat::INT12)
  {
    const unsigned long long len = str.size() * 16 / 12;
    if (len >= 0xffffffff)
    {
      mdcmAlwaysWarnMacro("RAWCodec: value too big for ByteValue");
      return false;
    }
    char * copy;
    try
    {
      copy = new char[len];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    const bool b = Unpack12Bits(copy, &str[0], str.size());
    if (!b)
    {
      delete[] copy;
      return false;
    }
    VL::Type lenSize = (VL::Type)len;
    out.SetByteValue(copy, lenSize);
    delete[] copy;
    this->GetPixelFormat().SetBitsAllocated(16);
  }
  else
  {
    const unsigned long long str_size = str.size();
    if (str_size >= 0xffffffff)
    {
      mdcmAlwaysWarnMacro("RAWCodec: value too big for ByteValue");
      return false;
    }
    out.SetByteValue(&str[0], (VL::Type)str_size);
  }
  return r;
}

bool
RAWCodec::GetHeaderInfo(std::istream &, TransferSyntax & ts)
{
  ts = TransferSyntax::ExplicitVRLittleEndian;
  if (NeedByteSwap)
  {
    ts = TransferSyntax::ImplicitVRBigEndianPrivateGE;
  }
  return true;
}

bool
RAWCodec::DecodeBytes(const char * inBytes, size_t inBufferLength, char * outBytes, size_t inOutBufferLength)
{
  if (!NeedByteSwap && !RequestPaddedCompositePixelCode && !RequestPlanarConfiguration &&
      PI != PhotometricInterpretation::YBR_FULL_422 && GetPixelFormat().GetBitsAllocated() != 12 && !NeedOverlayCleanup)
  {
    if (inOutBufferLength <= inBufferLength)
    {
      memcpy(outBytes, inBytes, inOutBufferLength);
    }
    else
    {
      mdcmWarningMacro("Truncating result");
      memcpy(outBytes, inBytes, inBufferLength);
    }
    return true;
  }
  std::stringstream is;
  is.write(inBytes, inBufferLength);
  std::stringstream os;
  const bool        r = DecodeByStreams(is, os);
  if (!r)
  {
    return false;
  }
  std::string str = os.str();
  if (this->GetPixelFormat() == PixelFormat::UINT12 || this->GetPixelFormat() == PixelFormat::INT12)
  {
    const size_t len = str.size() * 16 / 12;
    char *       copy;
    try
    {
      copy = new char[len];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    const bool b = Unpack12Bits(copy, &str[0], str.size());
    if (!b)
    {
      delete[] copy;
      return false;
    }
    memcpy(outBytes, copy, len);
    delete[] copy;
    this->GetPixelFormat().SetBitsAllocated(16);
  }
  else
  {
    memcpy(outBytes, str.c_str(), inOutBufferLength);
  }
  return r;
}

bool
RAWCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  return ImageCodec::DecodeByStreams(is, os);
}

} // end namespace mdcm
