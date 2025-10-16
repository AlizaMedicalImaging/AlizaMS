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
#include "mdcmUnpacker12Bits.h"
#include <limits>
#include <sstream>
#include <cstring>

namespace mdcm
{

bool
RAWCodec::CanCode(const TransferSyntax & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian || ts == TransferSyntax::ExplicitVRLittleEndian ||
          ts == TransferSyntax::ExplicitVRBigEndian || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE ||
          ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool
RAWCodec::CanDecode(const TransferSyntax & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian || ts == TransferSyntax::ExplicitVRLittleEndian ||
          ts == TransferSyntax::ExplicitVRBigEndian || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE ||
          ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool
RAWCodec::Code(const DataElement & in, DataElement & out)
{
  out = in;
  return true;
}

// This function is probably not used and could be removed
bool
RAWCodec::Decode(const DataElement & in, DataElement & out)
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
    const bool b = Unpacker12Bits::Unpack(copy, str.data(), str.size());
    if (!b)
    {
      delete[] copy;
      return false;
    }
    VL::Type lenSize = static_cast<VL::Type>(len);
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
    out.SetByteValue(str.data(), static_cast<VL::Type>(str_size));
  }
  return r;
}

bool
RAWCodec::GetHeaderInfo(std::istream &)
{
  // Removed guessing transfer syntax by header (unused),
  // commented for possible future implementation.
  /*
  ts = TransferSyntax::ExplicitVRLittleEndian;
  if (NeedByteSwap)
  {
    ts = TransferSyntax::ImplicitVRBigEndianPrivateGE;
  }
  */
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
      mdcmAlwaysWarnMacro("RAWCodec::DecodeBytes: can not truncate, inBufferLength=" << inBufferLength
                          << ", inOutBufferLength=" << inOutBufferLength);
#if 1
      return false;
#else
      memcpy(outBytes, inBytes, inBufferLength);
#endif
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
    if (inOutBufferLength != len)
    {
      mdcmDebugMacro("inOutBufferLength = " << inOutBufferLength
                     << ", inBufferLength = " << inBufferLength << ", len = " << len);
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
    const bool b = Unpacker12Bits::Unpack(copy, str.data(), str.size());
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
    const size_t len = str.size();
    if (inOutBufferLength <= len)
    {
      memcpy(outBytes, str.c_str(), inOutBufferLength);
    }
    else
    {
      mdcmAlwaysWarnMacro("RAWCodec::DecodeBytes: truncating" );
      memcpy(outBytes, str.c_str(), len);
    }
  }
  return r;
}

bool
RAWCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  return ImageCodec::DecodeByStreams(is, os);
}

} // end namespace mdcm
