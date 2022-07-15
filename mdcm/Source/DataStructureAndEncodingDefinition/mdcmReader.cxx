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
#include "mdcmReader.h"
#include "mdcmTrace.h"
#include "mdcmVR.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmSwapper.h"
#include "mdcmDeflateStream.h"
#include "mdcmSystem.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
#  include "mdcmUNExplicitDataElement.h"
#  include "mdcmCP246ExplicitDataElement.h"
#  include "mdcmExplicitImplicitDataElement.h"
#  include "mdcmUNExplicitImplicitDataElement.h"
#  include "mdcmVR16ExplicitDataElement.h"
#endif
#include <limits>

namespace mdcm
{

Reader::Reader()
  : F(new File)
{
  Stream = NULL;
  Ifstream = NULL;
}

Reader::~Reader()
{
  if (Ifstream)
  {
    Ifstream->close();
    delete Ifstream;
    Ifstream = NULL;
    Stream = NULL;
  }
}

bool
Reader::ReadPreamble()
{
  return true;
}

bool
Reader::ReadMetaInformation()
{
  return true;
}

bool
Reader::ReadDataSet()
{
  return true;
}

TransferSyntax
Reader::GuessTransferSyntax()
{
  // Don't call this function if there is a meta file info
  std::streampos                 start = Stream->tellg();
  SwapCode                       sc = SwapCode::Unknown;
  TransferSyntax::NegociatedType nts = TransferSyntax::Unknown;
  TransferSyntax                 ts(TransferSyntax::TS_END);
  Tag                            t;
  t.Read<SwapperNoOp>(*Stream);
  if (!(t.GetGroup() % 2))
  {
    switch (t.GetGroup())
    {
      case 0x0008:
        sc = SwapCode::LittleEndian;
        break;
      case 0x0800:
        sc = SwapCode::BigEndian;
        break;
      default:
        assert(0);
        break;
    }
    // Purposely not re-use ReadVR (can read VR_END)
    char vr_str[3];
    Stream->read(vr_str, 2);
    vr_str[2] = '\0';
    // Cannot use GetVRTypeFromFile
    VR::VRType vr = VR::GetVRType(vr_str);
    if (vr != VR::VR_END)
    {
      nts = TransferSyntax::Explicit;
    }
    else
    {
      assert(!(VR::IsSwap(vr_str)));
      Stream->seekg(-2, std::ios::cur); // Seek back
      if (t.GetElement() == 0x0000)
      {
        VL gl; // group length
        gl.Read<SwapperNoOp>(*Stream);
        switch (gl)
        {
          case 0x00000004:
            sc = SwapCode::LittleEndian; // 1234
            break;
          case 0x04000000:
            sc = SwapCode::BigEndian; // 4321
            break;
          case 0x00040000:
            sc = SwapCode::BadLittleEndian; // 3412
            mdcmWarningMacro("Bad Little Endian");
            break;
          case 0x00000400:
            sc = SwapCode::BadBigEndian; // 2143
            mdcmWarningMacro("Bad Big Endian");
            break;
          default:
            assert(0);
            break;
        }
      }
      nts = TransferSyntax::Implicit;
    }
  }
  else
  {
    mdcmWarningMacro("Start with a private tag creator");
    switch (t.GetElement())
    {
      case 0x0010:
        sc = SwapCode::LittleEndian;
        break;
      default:
        assert(0);
        break;
    }
    char vr_str[3];
    Stream->read(vr_str, 2);
    vr_str[2] = '\0';
    VR::VRType vr = VR::GetVRType(vr_str);
    if (vr != VR::VR_END)
    {
      nts = TransferSyntax::Explicit;
    }
    else
    {
      nts = TransferSyntax::Implicit;
      // Reading a private creator (0x0010) so it's LO, it's
      // difficult to come up with someting to check, maybe that
      // VL < 256
      mdcmWarningMacro("Very dangerous assertion"); // TODO
    }
  }
  assert(nts != TransferSyntax::Unknown);
  assert(sc != SwapCode::Unknown);
  if (nts == TransferSyntax::Implicit)
  {
    if (sc == SwapCode::BigEndian)
    {
      ts = TransferSyntax::ImplicitVRBigEndianACRNEMA;
    }
    else if (sc == SwapCode::LittleEndian)
    {
      ts = TransferSyntax::ImplicitVRLittleEndian;
    }
    else
    {
      assert(0);
    }
  }
  else
  {
    assert(0);
  }
  Stream->seekg(start, std::ios::beg);
  assert(ts != TransferSyntax::TS_END);
  return ts;
}

namespace details
{

class DefaultCaller
{
private:
  DataSet & m_dataSet;

public:
  DefaultCaller(DataSet & ds)
    : m_dataSet(ds)
  {}
  template <class T1, class T2>
  void
  ReadCommon(std::istream & is) const
  {
    m_dataSet.template Read<T1, T2>(is);
  }

  template <class T1, class T2>
  void
  ReadCommonWithLength(std::istream & is, VL & length) const
  {
    m_dataSet.template ReadWithLength<T1, T2>(is, length);
    // https://groups.google.com/forum/?fromgroups#!topic/comp.lang.c++/yTW4ESh1IL8
    is.setstate(std::ios::eofbit);
  }

  static void
  Check(bool b, std::istream & is)
  {
    if (b)
    {
      assert(is.eof());
    }
    (void)is;
  }
};

class ReadUpToTagCaller
{
private:
  DataSet &             m_dataSet;
  const Tag &           m_tag;
  std::set<Tag> const & m_skipTags;

public:
  ReadUpToTagCaller(DataSet & ds, const Tag & tag, std::set<Tag> const & skiptags)
    : m_dataSet(ds)
    , m_tag(tag)
    , m_skipTags(skiptags)
  {}

  template <class T1, class T2>
  void
  ReadCommon(std::istream & is) const
  {
    m_dataSet.template ReadUpToTag<T1, T2>(is, m_tag, m_skipTags);
  }

  template <class T1, class T2>
  void
  ReadCommonWithLength(std::istream & is, VL & length) const
  {
    m_dataSet.template ReadUpToTagWithLength<T1, T2>(is, m_tag, m_skipTags, length);
  }

  static void
  Check(bool, std::istream &)
  {}
};

class ReadSelectedTagsCaller
{
private:
  DataSet &             m_dataSet;
  std::set<Tag> const & m_tags;
  bool                  m_readvalues;

public:
  ReadSelectedTagsCaller(DataSet & ds, std::set<Tag> const & tags, const bool readvalues)
    : m_dataSet(ds)
    , m_tags(tags)
    , m_readvalues(readvalues)
  {}

  template <class T1, class T2>
  void
  ReadCommon(std::istream & is) const
  {
    m_dataSet.template ReadSelectedTags<T1, T2>(is, m_tags, m_readvalues);
  }

  template <class T1, class T2>
  void
  ReadCommonWithLength(std::istream & is, VL & length) const
  {
    m_dataSet.template ReadSelectedTagsWithLength<T1, T2>(is, m_tags, length, m_readvalues);
  }

  static void
  Check(bool, std::istream &)
  {}
};

class ReadSelectedPrivateTagsCaller
{
private:
  DataSet &                    m_dataSet;
  std::set<PrivateTag> const & m_groups;
  bool                         m_readvalues;

public:
  ReadSelectedPrivateTagsCaller(DataSet & ds, std::set<PrivateTag> const & groups, const bool readvalues)
    : m_dataSet(ds)
    , m_groups(groups)
    , m_readvalues(readvalues)
  {}

  template <class T1, class T2>
  void
  ReadCommon(std::istream & is) const
  {
    m_dataSet.template ReadSelectedPrivateTags<T1, T2>(is, m_groups, m_readvalues);
  }

  template <class T1, class T2>
  void
  ReadCommonWithLength(std::istream & is, VL & length) const
  {
    m_dataSet.template ReadSelectedPrivateTagsWithLength<T1, T2>(is, m_groups, length, m_readvalues);
  }

  static void
  Check(bool, std::istream &)
  {}
};

} // namespace details

bool
Reader::Read()
{
  details::DefaultCaller caller(F->GetDataSet());
  return InternalReadCommon(caller);
}

bool
Reader::ReadUpToTag(const Tag & tag, std::set<Tag> const & skiptags)
{
  details::ReadUpToTagCaller caller(F->GetDataSet(), tag, skiptags);
  return InternalReadCommon(caller);
}

bool
Reader::ReadSelectedTags(std::set<Tag> const & selectedTags, bool readvalues)
{
  details::ReadSelectedTagsCaller caller(F->GetDataSet(), selectedTags, readvalues);
  return InternalReadCommon(caller);
}

bool
Reader::ReadSelectedPrivateTags(std::set<PrivateTag> const & selectedPTags, bool readvalues)
{
  details::ReadSelectedPrivateTagsCaller caller(F->GetDataSet(), selectedPTags, readvalues);
  return InternalReadCommon(caller);
}

template <typename T_Caller>
bool
Reader::InternalReadCommon(const T_Caller & caller)
{
  if (!Stream || !*Stream)
  {
    mdcmErrorMacro("No File");
    return false;
  }
  bool success = true;
  try
  {
    std::istream & is = *Stream;
    bool           haspreamble = true;
    try
    {
      F->GetHeader().GetPreamble().Read(is);
    }
    catch (std::exception &)
    {
      // return to beginning of file, hopefully this file is simply missing preamble
      is.clear();
      is.seekg(0, std::ios::beg);
      haspreamble = false;
    }
    catch (...)
    {
      assert(0);
    }
    bool hasmetaheader = false;
    try
    {
      if (haspreamble)
      {
        try
        {
          F->GetHeader().Read(is);
          hasmetaheader = true;
          assert(!F->GetHeader().IsEmpty());
        }
        catch (std::exception & ex)
        {
          mdcmWarningMacro(ex.what());
          // weird implicit meta header
          is.seekg(128 + 4, std::ios::beg);
          assert(is.good());
          (void)ex;
          try
          {
            F->GetHeader().ReadCompat(is);
          }
          catch (std::exception & ex2)
          {
            // no meta header
            mdcmAlwaysWarnMacro(ex2.what());
          }
        }
      }
      else
      {
        F->GetHeader().ReadCompat(is);
      }
    }
    catch (std::exception &)
    {
      is.seekg(0, std::ios::beg);
      hasmetaheader = false;
    }
    catch (...)
    {
      assert(0);
    }
    if (F->GetHeader().IsEmpty())
    {
      hasmetaheader = false;
      mdcmDebugMacro("no file meta info found");
    }
    const TransferSyntax & ts = F->GetHeader().GetDataSetTransferSyntax();
    if (!ts.IsValid())
    {
      throw std::logic_error("Meta Header issue");
    }
    // Special case where the dataset was compressed using the deflate
    // algorithm
    if (ts == TransferSyntax::DeflatedExplicitVRLittleEndian)
    {
      zlib_stream::zip_istream gzis(is);
      assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
      caller.template ReadCommon<ExplicitDataElement, SwapperNoOp>(gzis);
      return is.good();
    }
    try
    {
      if (ts.GetSwapCode() == SwapCode::BigEndian)
      {
        // US-RGB-8-epicard.dcm is big endian
        if (ts.GetNegociatedType() == TransferSyntax::Implicit)
        {
          // LIBIDO-16-ACR_NEMA-Volume.dcm
          mdcmErrorMacro("VirtualBigEndianNotHandled");
          throw std::logic_error("Virtual Big Endian Implicit is not defined by DICOM");
        }
        else
        {
          caller.template ReadCommon<ExplicitDataElement, SwapperDoOp>(is);
        }
      }
      else // little endian
      {
        if (ts.GetNegociatedType() == TransferSyntax::Implicit)
        {
          if (hasmetaheader && haspreamble)
          {
            caller.template ReadCommon<ImplicitDataElement, SwapperNoOp>(is);
          }
          else
          {
            std::streampos start = is.tellg();
            is.seekg(0, std::ios::end);
            std::streampos end = is.tellg();
            assert(!is.eof());
            assert(is.good());
            std::streamoff theOffset = end - start;
            assert(theOffset > 0 || (uint32_t)theOffset < std::numeric_limits<uint32_t>::max());
            VL l = (uint32_t)(theOffset);
            is.seekg(start, std::ios::beg);
            assert(is.good());
            assert(!is.eof());
            caller.template ReadCommonWithLength<ImplicitDataElement, SwapperNoOp>(is, l);
          }
        }
        else
        {
          caller.template ReadCommon<ExplicitDataElement, SwapperNoOp>(is);
        }
      }
    }
    // Only catch parse exception at this point
    catch (ParseException & ex)
    {
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
      if (ex.GetLastElement().GetVR() == VR::UN && ex.GetLastElement().IsUndefinedLength())
      {
        // non CP 246
        is.clear();
        if (haspreamble)
        {
          is.seekg(128 + 4, std::ios::beg);
        }
        else
        {
          is.seekg(0, std::ios::beg);
        }
        if (hasmetaheader)
        {
          FileMetaInformation header;
          header.Read(is);
        }
        mdcmWarningMacro("Attempt to read non CP 246");
        F->GetDataSet().Clear();
        caller.template ReadCommon<CP246ExplicitDataElement, SwapperNoOp>(is);
      }
      else if (ex.GetLastElement().GetVR() == VR::UN)
      {
        is.clear();
        if (haspreamble)
        {
          is.seekg(128 + 4, std::ios::beg);
        }
        else
        {
          is.seekg(0, std::ios::beg);
        }
        if (hasmetaheader)
        {
          FileMetaInformation header;
          header.Read(is);
        }
        // GDCM 1.X
        mdcmWarningMacro("Attempt to read GDCM 1.X wrongly encoded");
        F->GetDataSet().Clear();
        caller.template ReadCommon<UNExplicitDataElement, SwapperNoOp>(is);
      }
      else if (ex.GetLastElement().GetTag() == Tag(0xfeff, 0x00e0))
      {
        // Famous philips where some private sequence were byteswapped
        // eg. PHILIPS_Intera-16-MONO2-Uncompress.dcm
        is.clear();
        if (haspreamble)
        {
          is.seekg(128 + 4, std::ios::beg);
        }
        else
        {
          is.seekg(0, std::ios::beg);
        }
        if (hasmetaheader)
        {
          FileMetaInformation header;
          header.Read(is);
        }
        mdcmWarningMacro("Attempt to read Philips with ByteSwap private sequence wrongly encoded");
        F->GetDataSet().Clear();
        assert(0); // TODO FIXME
      }
      else if (ex.GetLastElement().GetVR() == VR::INVALID)
      {
        if (ts.GetNegociatedType() == TransferSyntax::Explicit)
        {
          try
          {
            mdcmWarningMacro("Attempt to read file with VR16bits");
            // We could not read the VR in an explicit dataset
            // seek back tag + vr
            is.seekg(-6, std::ios::cur);
            VR16ExplicitDataElement ide;
            ide.template Read<SwapperNoOp>(is);
            // If we are here it means we succeeded in reading the implicit data element
            F->GetDataSet().Insert(ide);
            caller.template ReadCommon<VR16ExplicitDataElement, SwapperNoOp>(is);
          }
          catch (std::logic_error &)
          {
            try
            {
              // The file is neither:
              // 1. An Explicit encoded
              // 2. I could not reread it using the VR16Explicit reader, last option is that the file is
              // explicit/implicit
              is.clear();
              if (haspreamble)
              {
                is.seekg(128 + 4, std::ios::beg);
              }
              else
              {
                is.seekg(0, std::ios::beg);
              }
              if (hasmetaheader)
              {
                FileMetaInformation header;
                header.Read(is);
              }
              // Explicit/Implicit
              // mdcmData/c_vf1001.dcm falls into that category, while in fact the fmi could simply
              // be inverted and all would be perfect
              mdcmWarningMacro("Attempt to read file with explicit/implicit");
              F->GetDataSet().Clear();
              caller.template ReadCommon<ExplicitImplicitDataElement, SwapperNoOp>(is);
            }
            catch (std::exception &)
            {
              // MM: UNExplicitImplicitDataElement does not seems to be used anymore to read
              // mdcmData/TheralysMDCM120Bug.dcm, instead the code path goes into
              // ExplicitImplicitDataElement class instead.
              // Simply rethrow the exception for now.
              throw std::logic_error("Exception in Reader.cxx (1)"); // TODO
            }
          }
        }
      }
      else
      {
        mdcmWarningMacro("Attempt to read the file as mixture of explicit/implicit");
        // Try again with an ExplicitImplicitDataElement
        if (ts.GetSwapCode() == SwapCode::LittleEndian && ts.GetNegociatedType() == TransferSyntax::Explicit)
        {
          if (haspreamble)
          {
            is.seekg(128 + 4, std::ios::beg);
          }
          else
          {
            is.seekg(0, std::ios::beg);
          }
          if (hasmetaheader)
          {
            FileMetaInformation header;
            header.ReadCompat(is);
          }
          F->GetDataSet().Clear();
          caller.template ReadCommon<ExplicitImplicitDataElement, SwapperNoOp>(is);
        }
        else
        {
          mdcmWarningMacro("Exception in Reader.cxx (2)");
          success = false;
        }
      }
#else
      mdcmDebugMacro(ex.what());
      (void)ex;
      success = false;
#endif /* MDCM_SUPPORT_BROKEN_IMPLEMENTATION */
    }
    catch (...)
    {
      mdcmWarningMacro("Exception in Reader.cxx (3)");
      success = false;
    }
    caller.Check(success, *Stream);
  }
  catch (...)
  {
    mdcmWarningMacro("Exception in Reader.cxx (4)");
    success = false;
  }
  return success;
}

static inline bool
isasciiupper(char c)
{
  return c >= 'A' && c <= 'Z';
}

// This function re-implements code from:
// http://www.dclunie.com/medical-image-faq/html/part2.html#DICOMTransferSyntaxDetermination
// The above code does not work well for random file. It implicitly assumes we
// are trying to read a DICOM file in the first place, while our goal is indeed
// to detect whether or not the file can be assimilated as DICOM. So we
// extended it.  Of course this function only returns a 'maybe DICOM', since we
// are not guaranteed that the stream is not truncated, but this is outside the
// scope of this function.
bool
Reader::CanRead() const
{
  if (!Stream)
   return false;
  std::istream & is = *Stream;
  if (is.bad())
    return false;
  if (is.tellg() != std::streampos(0))
    return false;
  {
    char b[4];
    is.seekg(128, std::ios::beg);
    if (is.good() && is.read(b, 4) && strncmp(b, "DICM", 4) == 0)
    {
      is.seekg(0, std::ios::beg);
      return true;
    }
  }
  // Start overhead for backward compatibility
  bool bigendian = false;
  bool explicitvr = false;
  is.clear();
  is.seekg(0, std::ios::beg);
  char b[8];
  memset(b, 0, 8);
  if (is.good() && is.read(b, 8))
  {
    // examine probable group number, assume <= 0x00ff
    if (b[0] < b[1])
    {
      bigendian = true;
    }
    else if (b[0] == 0 && b[1] == 0)
    {
      // group number is zero
      // no point in looking at element number
      // as it will probably be zero too (group length)
      // try the 32 bit value length of implicit vr
      if (b[4] < b[7])
        bigendian = true;
    }
    // else littleendian
    if (isasciiupper(b[4]) && isasciiupper(b[5]))
      explicitvr = true;
  }
  else
  {
    is.clear();
    is.seekg(0, std::ios::beg);
    return false;
  }
  SwapCode                       sc = SwapCode::Unknown;
  TransferSyntax::NegociatedType nts = TransferSyntax::Unknown;
  std::stringstream              ss(std::string(b, 8));
  Tag                            t;
  if (bigendian)
  {
    t.Read<SwapperDoOp>(ss);
    if (t.GetGroup() <= 0xff)
      sc = SwapCode::BigEndian;
  }
  else
  {
    t.Read<SwapperNoOp>(ss);
    if (t.GetGroup() <= 0xff)
      sc = SwapCode::LittleEndian;
  }
  VL         vl;
  VR::VRType vr = VR::VR_END;
  if (explicitvr)
  {
    char vr_str[3];
    vr_str[0] = b[4];
    vr_str[1] = b[5];
    vr_str[2] = '\0';
    vr = VR::GetVRType(vr_str);
    if (vr != VR::VR_END)
      nts = TransferSyntax::Explicit;
  }
  else
  {
    if (bigendian)
      vl.Read<SwapperDoOp>(ss);
    else
      vl.Read<SwapperNoOp>(ss);
    if (vl < 0xff)
      nts = TransferSyntax::Implicit;
  }
  // reset
  is.clear();
  is.seekg(0, std::ios::beg);
  // Implicit Little Endian
  if (nts == TransferSyntax::Implicit && sc == SwapCode::LittleEndian)
    return true;
  if (nts == TransferSyntax::Implicit && sc == SwapCode::BigEndian)
    return false;
  if (nts == TransferSyntax::Explicit && sc == SwapCode::LittleEndian)
    return true;
  if (nts == TransferSyntax::Explicit && sc == SwapCode::BigEndian)
    return true;
  return false;
}

void
Reader::SetFileName(const char * p)
{
  if (Ifstream)
    delete Ifstream;
  Ifstream = new std::ifstream();
  if (p && *p)
  {
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
    const std::wstring uncpath = mdcm::System::ConvertToUtf16(p);
    Ifstream->open(uncpath.c_str(), std::ios_base::in | std::ios::binary);
#else
    Ifstream->open(p, std::ios::binary);
#endif
  }
  else
  {
    mdcmAlwaysWarnMacro("Reader failed (1)");
  }
  if (Ifstream->is_open())
  {
    Stream = Ifstream;
    assert(Stream && *Stream);
  }
  else
  {
    mdcmAlwaysWarnMacro("Reader failed (2)");
    delete Ifstream;
    Ifstream = NULL;
    Stream = NULL;
  }
}

#if 0
size_t Reader::GetStreamCurrentPosition() const
{
  return static_cast<size_t>(GetStreamPtr()->tellg());
}
#endif

} // end namespace mdcm
