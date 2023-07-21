/*********************************************************
 *
 * MDCM
 *
 * github.com/issakomi
 *
 *********************************************************/

#include "mdcmEncapsulatedRAWCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmByteSwap.h"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include <limits>
#include <sstream>
#include <cstring>

namespace mdcm
{

EncapsulatedRAWCodec::EncapsulatedRAWCodec() {}

EncapsulatedRAWCodec::~EncapsulatedRAWCodec() {}

bool
EncapsulatedRAWCodec::CanCode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::EncapsulatedUncompressedExplicitVRLittleEndian);
}

bool
EncapsulatedRAWCodec::CanDecode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::EncapsulatedUncompressedExplicitVRLittleEndian);
}

bool
EncapsulatedRAWCodec::Code(const char * in, unsigned long long len, DataElement & out)
{
  const unsigned int * dims = this->GetDimensions();
  if ((len % dims[2]) != 0)
  {
    mdcmAlwaysWarnMacro("(len % dims[2]) != 0, " << len << " % " << dims[2]
                                                 << " = " << len % dims[2]);
    return false;
  }
  const PixelFormat & pf = this->GetPixelFormat();
  if (pf.GetBitsAllocated() % 8 != 0)
  {
    mdcmAlwaysWarnMacro("pf.GetBitsAllocated() % 8 != 0");
    return false;
  }
  const size_t        frag_len = len / dims[2];
  const size_t        frag_len2 = static_cast<size_t>(dims[0]) * dims[1] * pf.GetSamplesPerPixel() * (pf.GetBitsAllocated() / 8);
  if (frag_len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("fragment size is too big");
  }
  if (frag_len != frag_len2)
  {
    mdcmAlwaysWarnMacro("frag_len != frag_len2, " << frag_len << " != " << frag_len2);
    return false;
  }
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  for (size_t j = 0; j < dims[2]; ++j)
  {
    const char * data = in + j * frag_len;
    Fragment frag;
    frag.SetByteValue(data, static_cast<unsigned int>(frag_len));
    sq->AddFragment(frag);
  }
  if (sq->GetNumberOfFragments() != dims[2])
  {
    mdcmAlwaysWarnMacro("Number of fragments " << sq->GetNumberOfFragments()
                                               << " != dims[2] "
                                               << dims[2]);
    return false;
  }
  out.SetValue(*sq);
  return true;
}

bool
EncapsulatedRAWCodec::Decode2(DataElement const & in, char * out_buffer, unsigned long long out_len)
{
  if (NeedByteSwap)
  {
    mdcmAlwaysWarnMacro("NeedByteSwap is currently not supported for EncapsulatedRAWCodec");
    return false;
  }
  const SequenceOfFragments * sf = in.GetSequenceOfFragments();
  if (!sf)
    return false;
  const size_t n = sf->GetNumberOfFragments();
  if (sf->GetNumberOfFragments() != Dimensions[2])
    return false;
  std::stringstream os;
  for (size_t i = 0; i < n; ++i)
  {
    const Fragment & frag = sf->GetFragment(i);
    if (frag.IsEmpty())
      return false;
    const ByteValue * bv = frag.GetByteValue();
    if (!bv)
      return false;
    if (!bv->GetPointer())
      return false;
    const size_t len = bv->GetLength();
    const char * buffer = bv->GetPointer();
    os.write(buffer, len);
  }
  const size_t len2 = os.tellp();
  if (len2 != out_len)
  {
    mdcmAlwaysWarnMacro("EncapsulatedRAWCodec::Decode2: out_len=" << out_len << " len2=" << len2);
    if (out_len > len2)
    {
      memset(out_buffer, 0, out_len);
    }
  }
  os.seekp(0, std::ios::beg);
#if 1
  std::stringbuf * pbuf = os.rdbuf();
  pbuf->sgetn(out_buffer, ((out_len < len2) ? out_len : len2));
#else
  const std::string & tmp0 = os.str();
  const char * tmp1 = tmp0.data();
  memcpy(
    out_buffer,
    tmp1,
    ((out_len < len2) ? out_len : len2));
#endif
  return true;
}

bool
EncapsulatedRAWCodec::GetHeaderInfo(std::istream &, TransferSyntax & ts)
{
  ts = TransferSyntax::EncapsulatedUncompressedExplicitVRLittleEndian;
  return true;
}

bool
EncapsulatedRAWCodec::DecodeBytes(const char *, size_t, char *, size_t)
{
  return false;
}

bool
EncapsulatedRAWCodec::DecodeByStreams(std::istream &, std::ostream &)
{
  return false;
}

} // end namespace mdcm
