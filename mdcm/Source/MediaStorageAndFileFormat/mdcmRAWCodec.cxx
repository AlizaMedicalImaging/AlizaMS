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
#include "mdcmByteSwap.txx"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmUnpacker12Bits.h"
#include <limits>
#include <sstream>
#include <cstring>

namespace mdcm
{

class RAWInternals
{
public:
};

RAWCodec::RAWCodec()
{
  Internals = new RAWInternals;
}

RAWCodec::~RAWCodec()
{
  delete Internals;
}

bool RAWCodec::CanCode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian
       || ts == TransferSyntax::ExplicitVRLittleEndian
       || ts == TransferSyntax::ExplicitVRBigEndian
       || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE
       || ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool RAWCodec::CanDecode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::ImplicitVRLittleEndian
       || ts == TransferSyntax::ExplicitVRLittleEndian
       || ts == TransferSyntax::ExplicitVRBigEndian
       || ts == TransferSyntax::ImplicitVRBigEndianPrivateGE
       || ts == TransferSyntax::DeflatedExplicitVRLittleEndian);
}

bool RAWCodec::Code(DataElement const & in, DataElement & out)
{
  out = in;
  return true;
}

bool RAWCodec::DecodeBytes(
  const char * inBytes, size_t inBufferLength,
  char *      outBytes, size_t inOutBufferLength)
{
  if(!NeedByteSwap && !RequestPaddedCompositePixelCode &&
     !RequestPlanarConfiguration &&
	 PI != PhotometricInterpretation::YBR_FULL_422 &&
     GetPixelFormat().GetBitsAllocated() != 12 &&
     !NeedOverlayCleanup)
  {
    if(inOutBufferLength <= inBufferLength)
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
  const bool r = DecodeByStreams(is, os);
  if(!r) return false;
  std::string str = os.str();
  if(this->GetPixelFormat() == PixelFormat::UINT12 ||
     this->GetPixelFormat() == PixelFormat::INT12)
  {
    const size_t len = str.size() * 16 / 12;
    char * copy = new char[len];
    const bool b = Unpacker12Bits::Unpack(copy, &str[0], str.size());
    if (!b)
    {
      delete [] copy;
      return false;
    }
    memcpy(outBytes, copy, len);
    delete [] copy;
    this->GetPixelFormat().SetBitsAllocated(16);
  }
  else
  {
    memcpy(outBytes, str.c_str(), inOutBufferLength);
  }
  return r;
}

bool RAWCodec::Decode(DataElement const & in, DataElement & out)
{
  if(!NeedByteSwap &&
     !RequestPaddedCompositePixelCode &&
     PI == PhotometricInterpretation::MONOCHROME2 &&
     !PlanarConfiguration && !RequestPlanarConfiguration &&
     GetPixelFormat().GetBitsAllocated() != 12 &&
     !NeedOverlayCleanup)
  {
    out = in;
    return true;
  }
  const ByteValue * bv = in.GetByteValue();
  if (!bv) return false;
  std::stringstream is;
  is.write(bv->GetPointer(), bv->GetLength());
  std::stringstream os;
  const bool r = DecodeByStreams(is, os);
  if(!r) return false;
  std::string str = os.str();
  out = in;
  if(this->GetPixelFormat() == PixelFormat::UINT12 ||
     this->GetPixelFormat() == PixelFormat::INT12)
  {
    const size_t len = str.size() * 16 / 12;
    char * copy = new char[len];
    const bool b = Unpacker12Bits::Unpack(copy, &str[0], str.size());
    if (!b)
    {
      delete [] copy;
      return false;
    }
    VL::Type lenSize = (VL::Type)len;
    out.SetByteValue(copy, lenSize);
    delete [] copy;
    this->GetPixelFormat().SetBitsAllocated(16);
  }
  else
  {
    VL::Type strSize = (VL::Type) str.size();
    out.SetByteValue(&str[0], strSize);
  }
  return r;
}

bool RAWCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  return ImageCodec::DecodeByStreams(is, os);
}

bool RAWCodec::GetHeaderInfo(std::istream &, TransferSyntax & ts)
{
  ts = TransferSyntax::ExplicitVRLittleEndian;
  if(NeedByteSwap)
  {
    ts = TransferSyntax::ImplicitVRBigEndianPrivateGE;
  }
  return true;
}

ImageCodec * RAWCodec::Clone() const
{
  return NULL;
}

} // end namespace mdcm
