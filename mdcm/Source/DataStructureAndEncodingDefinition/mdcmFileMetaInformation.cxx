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
#include "mdcmFileMetaInformation.h"
#include "mdcmAttribute.h"
#include "mdcmVR.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmByteValue.h"
#include "mdcmSwapper.h"
#include "mdcmTagToType.h"

#include "mdcmTag.h"

namespace mdcm
{

const char FileMetaInformation::MDCM_FILE_META_INFORMATION_VERSION[] = "\0\1";

const char FileMetaInformation::MDCM_IMPLEMENTATION_CLASS_UID[] = "1.2.826.0.1.3680043.10.135.1." MDCM_VERSION;
const char FileMetaInformation::MDCM_IMPLEMENTATION_VERSION_NAME[] = "MDCM " MDCM_VERSION;
const char FileMetaInformation::MDCM_SOURCE_APPLICATION_ENTITY_TITLE[] = "MDCM";
std::string FileMetaInformation::ImplementationClassUID = GetMDCMImplementationClassUID();
std::string FileMetaInformation::ImplementationVersionName = GetMDCMImplementationVersionName();
std::string FileMetaInformation::SourceApplicationEntityTitle = GetMDCMSourceApplicationEntityTitle();

const char * FileMetaInformation::GetFileMetaInformationVersion()
{
  return MDCM_FILE_META_INFORMATION_VERSION;
}

const char * FileMetaInformation::GetMDCMImplementationClassUID()
{
  return MDCM_IMPLEMENTATION_CLASS_UID;
}

const char * FileMetaInformation::GetMDCMImplementationVersionName()
{
  return MDCM_IMPLEMENTATION_VERSION_NAME;
}

const char * FileMetaInformation::GetMDCMSourceApplicationEntityTitle()
{
  return MDCM_SOURCE_APPLICATION_ENTITY_TITLE;
}

// Keep cstor and dstor here to keep API minimal (see dllexport issue with mdcmstrict::)
FileMetaInformation::FileMetaInformation() :
  DataSetTS(TransferSyntax::TS_END),
  MetaInformationTS(TransferSyntax::Unknown),
  DataSetMS(MediaStorage::MS_END) {}
FileMetaInformation::~FileMetaInformation() {}

void FileMetaInformation::SetImplementationClassUID(const char * imp)
{
  // TODO: it would be nice to make sure imp is actually a valid UID
  if(imp)
  {
    ImplementationClassUID = imp;
  }
}

void FileMetaInformation::AppendImplementationClassUID(const char * imp)
{
  if(imp)
  {
    ImplementationClassUID = GetMDCMImplementationClassUID();
    ImplementationClassUID += ".";
    ImplementationClassUID += imp;
  }
}

void FileMetaInformation::SetImplementationVersionName(const char * version)
{
  if(version)
  {
    mdcmAssertAlwaysMacro(strlen(version) <= 16);
    ImplementationVersionName = version;
  }
}

void FileMetaInformation::SetSourceApplicationEntityTitle(const char * title)
{
  if(title)
  {
    AEComp ae(title);
    SourceApplicationEntityTitle = ae.Truncate();
  }
}

const char * FileMetaInformation::GetImplementationClassUID()
{
  return ImplementationClassUID.c_str();
}

const char * FileMetaInformation::GetImplementationVersionName()
{
  return ImplementationVersionName.c_str();
}

const char * FileMetaInformation::GetSourceApplicationEntityTitle()
{
  return SourceApplicationEntityTitle.c_str();
}

void FileMetaInformation::FillFromDataSet(DataSet const & ds)
{
  DataElement xde;
  if(!FindDataElement(Tag(0x0002, 0x0001)))
  {
    xde.SetTag(Tag(0x0002, 0x0001));
    xde.SetVR(VR::OB);
    const char *version = FileMetaInformation::GetFileMetaInformationVersion();
    xde.SetByteValue(version, 2);
    Insert(xde);
  }
  else
  {
    const DataElement & de = GetDataElement(Tag(0x0002,0x0001));
    const ByteValue * bv = de.GetByteValue();
    if(bv->GetLength() != 2 ||
      (memcmp(bv->GetPointer(), FileMetaInformation::GetFileMetaInformationVersion(), 2) != 0))
    {
      xde.SetTag(Tag(0x0002, 0x0001));
      xde.SetVR(VR::OB);
      const char *version = FileMetaInformation::GetFileMetaInformationVersion();
      xde.SetByteValue(version, 2);
      Replace(xde);
    }
  }
  if(!FindDataElement(Tag(0x0002, 0x0002)) || GetDataElement(Tag(0x0002,0x0002)).IsEmpty())
  {
    if(!ds.FindDataElement(Tag(0x0008, 0x0016)) || ds.GetDataElement(Tag(0x0008,0x0016)).IsEmpty() )
    {
      MediaStorage ms;
      ms.SetFromModality(ds);
      const char * msstr = ms.GetString();
      if(msstr)
      {
        VL::Type strlenMsstr = (VL::Type) strlen(msstr);
        xde.SetByteValue(msstr, strlenMsstr);
        xde.SetTag(Tag(0x0002, 0x0002));
        {
          xde.SetVR(VR::UI);
        }
        Insert(xde);
      }
      else
      {
        mdcmErrorMacro("Could not find MediaStorage");
      }
    }
    else
    {
      const DataElement & msclass = ds.GetDataElement(Tag(0x0008, 0x0016));
      xde = msclass;
      xde.SetTag(Tag(0x0002, 0x0002));
      if(msclass.GetVR() == VR::UN || msclass.GetVR() == VR::INVALID)
      {
        xde.SetVR(VR::UI);
      }
      Insert(xde);
    }
  }
  else // Ok there is a value in (0002,0002) let see if it match (0008,0016)
  {
    {
      if(!ds.FindDataElement(Tag(0x0008, 0x0016)))
      {
        mdcmWarningMacro("Missing SOPClassUID in DataSet but found in FileMeta");
      }
      else
      {
        const DataElement& sopclass = ds.GetDataElement(Tag(0x0008, 0x0016));
        DataElement mssopclass = GetDataElement(Tag(0x0002, 0x0002));
        assert(!mssopclass.IsEmpty());
        const ByteValue * bv = sopclass.GetByteValue();
        if(bv)
        {
          mssopclass.SetByteValue(bv->GetPointer(), bv->GetLength());
          Replace(mssopclass);
        }
        else
        {
          throw std::logic_error("SOP Class is empty");
        }
      }
    }
  }
  // Media Storage SOP Instance UID (0002,0003) -> see (0008,0018)
  const DataElement &dummy = GetDataElement(Tag(0x0002,0x0003)); (void)dummy;
  if(!FindDataElement(Tag(0x0002, 0x0003)) || GetDataElement(Tag(0x0002,0x0003)).IsEmpty())
  {
    if(ds.FindDataElement(Tag(0x0008, 0x0018)))
    {
      const DataElement & msinst = ds.GetDataElement(Tag(0x0008, 0x0018));
      xde = msinst;
      xde.SetTag(Tag(0x0002, 0x0003));
      if(msinst.GetVR() == VR::UN || msinst.GetVR() == VR::INVALID)
      {
        xde.SetVR(VR::UI);
      }
      Replace(xde);
    }
    else
    {
      throw std::logic_error("No (0x0002,0x0003) and (0x0008,0x0018) elements");
    }
  }
  else // Ok there is a value in (0002,0003) let see if it match (0008,0018)
  {
    bool dirrecsq = ds.FindDataElement(Tag(0x0004, 0x1220)); // Directory Record Sequence
    MediaStorage ms;
    ms.SetFromHeader(*this);
    bool dicomdir = (ms == MediaStorage::MediaStorageDirectoryStorage && dirrecsq);
    if(!dicomdir)
    {
      if(!ds.FindDataElement(Tag(0x0008, 0x0018)) ||
         ds.GetDataElement(Tag(0x0008, 0x0018)).IsEmpty())
      {
        throw std::logic_error("No (0x0008,0x0018) element");
      }
      const DataElement & sopinst = ds.GetDataElement(Tag(0x0008, 0x0018));
      assert(!GetDataElement(Tag(0x0002, 0x0003)).IsEmpty());
      DataElement mssopinst = GetDataElement(Tag(0x0002, 0x0003));
      const ByteValue * bv = sopinst.GetByteValue();
      if (bv)
      {
        mssopinst.SetByteValue(bv->GetPointer(), bv->GetLength());
        Replace(mssopinst);
      }
    }
  }
  // Transfer Syntax UID (0002,0010) -> ??? (computed at write time at most)
  if(FindDataElement(Tag(0x0002, 0x0010)) && !GetDataElement(Tag(0x0002,0x0010)).IsEmpty())
  {
    DataElement tsuid = GetDataElement(Tag(0x0002, 0x0010));
    const char * datasetts = DataSetTS.GetString();
    const ByteValue * bv = tsuid.GetByteValue();
    assert(bv);
    std::string currentts(bv->GetPointer(), bv->GetPointer() + bv->GetLength());
    if(strlen(currentts.c_str()) != strlen(datasetts)
      || strcmp(currentts.c_str(), datasetts) != 0)
    {
      xde = tsuid;
      VL::Type strlenDatasetts = (VL::Type) strlen(datasetts);
      xde.SetByteValue(datasetts, strlenDatasetts);
      Replace(xde);
    }
    if(tsuid.GetVR() != VR::UI)
    {
      xde = tsuid;
      xde.SetVR(VR::UI);
      Replace(xde);
    }
  }
  else
  {
    // Very bad !!
    // Constuct it from DataSetTS
    if(DataSetTS == TransferSyntax::TS_END)
    {
      throw std::logic_error("No TransferSyntax specified");
    }
    const char* str = TransferSyntax::GetTSString(DataSetTS);
    VL::Type strlenStr = (VL::Type) strlen(str);
    xde.SetByteValue(str, strlenStr);
    xde.SetVR(VR::UI);
    xde.SetTag(Tag(0x0002,0x0010));
    Insert(xde);
  }
  // Implementation Class UID (0002,0012) -> ??
  if(!FindDataElement(Tag(0x0002, 0x0012)))
  {
    xde.SetTag(Tag(0x0002, 0x0012));
    xde.SetVR(VR::UI);
    //const char implementation[] = MDCM_IMPLEMENTATION_CLASS_UID;
    const char *implementation = FileMetaInformation::GetImplementationClassUID();
    VL::Type strlenImplementation = (VL::Type) strlen(implementation);
    xde.SetByteValue(implementation, strlenImplementation);
    Insert(xde);
  }
  else
  {
    // TODO: Need to check Implementation UID is actually a valid UID...
  }
  // Implementation Version Name (0002,0013) -> ??
  if(!FindDataElement(Tag(0x0002, 0x0013)))
  {
    xde.SetTag(Tag(0x0002, 0x0013));
    xde.SetVR(VR::SH);
    //const char version[] = MDCM_IMPLEMENTATION_VERSION_NAME;
    SHComp version = FileMetaInformation::GetImplementationVersionName();
    VL::Type strlenVersion = (VL::Type)strlen(version);
    xde.SetByteValue(version, strlenVersion);
    Insert(xde);
  }
  // Source Application Entity Title (0002,0016) -> ??
  if(!FindDataElement(Tag(0x0002, 0x0016)))
  {
    xde.SetTag(Tag(0x0002, 0x0016));
    xde.SetVR(VR::AE);
    const char *title = FileMetaInformation::GetSourceApplicationEntityTitle();
    VL::Type strlenTitle = (VL::Type)strlen(title);
    xde.SetByteValue(title, strlenTitle);
    Insert(xde);
  }
  // Do this one last!
  // (Meta) Group Length (0002,0000) -> computed
  Attribute<0x0002, 0x0000> filemetagrouplength;
  Remove(filemetagrouplength.GetTag());
  unsigned int glen = GetLength<ExplicitDataElement>();
  assert((glen % 2) == 0);
  filemetagrouplength.SetValue(glen);
  Insert(filemetagrouplength.GetAsDataElement());

  assert(!IsEmpty());
}

// FIXME
// This code should clearly be rewritten with some template meta programing to
// enable reuse of code
//
template <typename TSwap>
bool ReadExplicitDataElement(std::istream &is, ExplicitDataElement &de)
{
  // Read Tag
  std::streampos start = is.tellg();
  Tag t;
  if(!t.template Read<TSwap>(is))
  {
    assert(0 && "Should not happen");
    return false;
  }
  if(t.GetGroup() != 0x0002)
  {
    std::streampos currentpos = is.tellg();
    // old code was fseeking from the beginning of file
    // which seems to be quite different than fseeking in reverse from
    // the current position?
    assert((start - currentpos) <= 0);
    assert((int)(start - currentpos) == -4);
    is.seekg((start - currentpos), std::ios::cur);
    return false;
  }
  // Read VR
  VR vr;
  if(!vr.Read(is))
  {
    is.seekg(start, std::ios::beg);
    return false;
  }
  // Read Value Length
  VL vl;
  if(vr & VR::VL32)
  {
    if(!vl.template Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return false;
    }
  }
  else
  {
    // Value Length is stored on 16bits only
    vl.template Read16<TSwap>(is);
  }
  // Read the Value
  ByteValue *bv = NULL;
  if(vr == VR::SQ)
  {
    assert(0 && "Should not happen");
    return false;
  }
  else if(vl.IsUndefined())
  {
    assert(0 && "Should not happen");
    return false;
  }
  else
  {
    bv = new ByteValue;
  }
  // We have the length we should be able to read the value
  bv->SetLength(vl); // perform realloc
  if(!bv->template Read<TSwap>(is))
  {
    assert(0 && "Should not happen");
    return false;
  }
  assert(bv->GetLength() == vl);
  de.SetTag(t);
  de.SetVR(vr);
  de.SetVL(vl);
  // FIXME: There should be a way to set the Value to the NULL pointer...
  de.SetValue(*bv);
  return true;
}

template <typename TSwap>
bool ReadImplicitDataElement(std::istream &is, ImplicitDataElement &de)
{
  // See PS 3.5, 7.1.3 Data Element Structure With Implicit VR
  std::streampos start = is.tellg();
  // Read Tag
  Tag t;
  if(!t.template Read<TSwap>(is))
  {
    assert(0 && "Should not happen");
    return false;
  }
  if(t.GetGroup() != 0x0002)
  {
    mdcmDebugMacro("Done reading File Meta Information");
    is.seekg(start, std::ios::beg);
    return false;
  }
  // Read Value Length
  VL vl;
  if(!vl.template Read<TSwap>(is))
  {
    assert(0 && "Should not happen");
    return false;
  }
  ByteValue *bv = 0;
  if(vl.IsUndefined())
  {
    assert(0 && "Should not happen");
    return false;
  }
  else
  {
    bv = new ByteValue;
  }
  // We have the length we should be able to read the value
  bv->SetLength(vl); // perform realloc
  if(!bv->template Read<TSwap>(is))
  {
    assert(0 && "Should not happen");
    return false;
  }
  de.SetTag(t);
  de.SetVL(vl);
  de.SetValue(*bv);

  return true;
}

/*
 * Except for the 128 bytes preamble and the 4 bytes prefix, the File Meta
 * Information shall be encoded using the Explicit VR Little Endian Transfer
 * Syntax (UID=1.2.840.10008.1.2.1) as defined in DICOM PS 3.5.
 * Values of each File Meta Element shall be padded when necessary to achieve
 * an even length as specified in PS 3.5 by their corresponding Value
 * Representation. For compatibility with future versions of this Standard,
 * any Tag (0002,xxxx) not defined in Table 7.1-1 shall be ignored.
 * Values of all Tags (0002,xxxx) are reserved for use by this Standard and
 * later versions of DICOM.
 * Note: PS 3.5 specifies that Elements with Tags (0001,xxxx), (0003,xxxx),
 * (0005,xxxx), and (0007,xxxx) shall not be used.
 */
/// \TODO FIXME
/// For now I do a Seek back of 6 bytes. It would be better to finish reading
/// the first element of the FMI so that I can read the group length and
/// therefore compare it against the actual value we found...
std::istream &FileMetaInformation::Read(std::istream &is)
{
  std::streampos start = is.tellg();
  // TODO: Can now load data from std::ios::cur to std::ios::cur + metagl.GetValue()

  ExplicitDataElement xde;
  Tag gl;
  gl.Read<SwapperNoOp>(is);
  if(gl.GetGroup() != 0x2)
  {
    throw std::logic_error("Group is invalid");
  }
  if(gl.GetElement() != 0x0)
  {
    throw std::logic_error("Element is invalid");
  }
  VR vr;
  vr.Read(is);
  if(vr == VR::INVALID)
  {
    throw std::logic_error("VR is invalid");
  }
  if(vr != VR::UL)
  {
    throw std::logic_error("VR is invalid (!UL)");
  }
  // TODO FIXME: I should not do seekg for valid file this is costly
  is.seekg(-6,std::ios::cur);
  xde.Read<SwapperNoOp>(is);
  Insert(xde);
  // See PS 3.5, Data Element Structure With Explicit VR
  try
  {
    // MDCM is a hack, so let's read all possible group 2 element, until the last one
    // and leave the group length value aside.
    while(ReadExplicitDataElement<SwapperNoOp>(is, xde))
    {
      Insert(xde);
    }
  }
  catch(std::exception & ex)
  {
    (void)ex;
    // we've read a little bit too much. We are possibly in the case where an
    // implicit dataelement with group 2 (technically impossible) was found
    // (first dataelement). Let's start over again, but this time use group
    // length as the sentinel for the last group 2 element:
    is.seekg(start,std::ios::beg);
    // Group Length:
    ReadExplicitDataElement<SwapperNoOp>(is, xde);
    Attribute<0x0002, 0x0000> filemetagrouplength;
    filemetagrouplength.SetFromDataElement(xde);
    const unsigned int glen = filemetagrouplength.GetValue();
    unsigned int cur_len = 0;
    while(cur_len < glen && ReadExplicitDataElement<SwapperNoOp>(is, xde))
    {
      Insert(xde);
      cur_len += xde.GetLength();
    }
  }
  // Now is a good time to compute the transfer syntax:
  ComputeDataSetTransferSyntax();
  // we are at the end of the meta file information and before the dataset
  return is;
}

std::istream &FileMetaInformation::ReadCompat(std::istream & is)
{
  assert(is.good());
  // First off save position in case we fail (no File Meta Information)
  // See PS 3.5, Data Element Structure With Explicit VR
  if(!IsEmpty())
  {
    throw std::logic_error("!IsEmpty()");
  }
  Tag t;
  if(!t.Read<SwapperNoOp>(is))
  {
    throw std::logic_error("Cannot read very first tag");
  }
  if(t.GetGroup() == 0x0002)
  {
    // GE_DLX-8-MONO2-PrivateSyntax.dcm is in Implicit...
    return ReadCompatInternal<SwapperNoOp>(is);
  }
  else if(t.GetGroup() == 0x0008)
  {
    char vr_str[3];
    is.read(vr_str, 2);
    vr_str[2] = '\0';
    VR::VRType vr = VR::GetVRType(vr_str);
    if(vr != VR::VR_END)
    {
      // File start with a 0x0008 element but no FileMetaInfo and is Explicit
      DataSetTS = TransferSyntax::ExplicitVRLittleEndian;
    }
    else
    {
      // File start with a 0x0008 element but no FileMetaInfo and is Implicit
      DataSetTS = TransferSyntax::ImplicitVRLittleEndian;
    }
    is.seekg(-6, std::ios::cur); // Seek back
  }
  else if(t.GetGroup() == 0x0800) // Good ol' ACR NEMA
  {
    char vr_str[3];
    is.read(vr_str, 2);
    vr_str[2] = '\0';
    VR::VRType vr = VR::GetVRType(vr_str);
    if(vr != VR::VR_END)
    {
      // File start with a 0x0008 element but no FileMetaInfo and is Explicit
      DataSetTS = TransferSyntax::ExplicitVRBigEndian;
    }
    else
    {
      // File start with a 0x0008 element but no FileMetaInfo and is Implicit
      DataSetTS = TransferSyntax::ImplicitVRBigEndianACRNEMA;
    }
    is.seekg(-6, std::ios::cur); // Seek back
  }
  else if(t.GetElement() == 0x0010) // Hum, is it a private creator ?
  {
    char vr_str[3];
    is.read(vr_str, 2);
    vr_str[2] = '\0';
    VR::VRType vr = VR::GetVRType(vr_str);
    if(vr != VR::VR_END)
    {
      DataSetTS = TransferSyntax::ExplicitVRLittleEndian;
    }
    else
    {
      DataSetTS = TransferSyntax::ImplicitVRLittleEndian;
    }
    is.seekg(-6, std::ios::cur); // Seek back
  }
  else
  {
    char vr_str[3];
    VR::VRType vr = VR::VR_END;
    if(is.read(vr_str, 2))
    {
      vr_str[2] = '\0';
      vr = VR::GetVRType(vr_str);
    }
    else
    {
      throw std::logic_error("Can not read 2 bytes for VR");
    }
    is.seekg(-6, std::ios::cur); // Seek back
    if(vr != VR::VR_END)
    {
      // Ok we found a VR, this is 99% likely to be our safe bet
      if(t.GetGroup() > 0xff || t.GetElement() > 0xff)
        DataSetTS = TransferSyntax::ExplicitVRBigEndian;
      else
        DataSetTS = TransferSyntax::ExplicitVRLittleEndian;
    }
    else
    {
      DataElement null(Tag(0x0,0x0), 0);
      ImplicitDataElement ide;
      ide.ReadPreValue<SwapperNoOp>(is);
      if(ide.GetTag() == null.GetTag() && ide.GetVL() == 4)
      {
        // This is insane, we are actually reading an attribute with tag (0,0) !
        // something like IM-0001-0066.CommandTag00.dcm was crafted
        ide.ReadValue<SwapperNoOp>(is);
        ReadCompat(is); // this will read the next element
        assert(DataSetTS == TransferSyntax::ImplicitVRLittleEndian);
        is.seekg(-12, std::ios::cur); // Seek back
        return is;
      }
      else
      {
        throw std::logic_error("Can not distinguish type of DICOM file");
      }
    }
  }
  return is;
}

#define ADDVRIMPLICIT(element) \
    case element: \
      de.SetVR((VR::VRType)TagToType<0x0002,element>::VRType); \
      break

bool AddVRToDataElement(DataElement &de)
{
  switch(de.GetTag().GetElement())
  {
    ADDVRIMPLICIT(0x0000);
    ADDVRIMPLICIT(0x0001);
    ADDVRIMPLICIT(0x0002);
    ADDVRIMPLICIT(0x0003);
    ADDVRIMPLICIT(0x0010);
    ADDVRIMPLICIT(0x0012);
    ADDVRIMPLICIT(0x0013);
    ADDVRIMPLICIT(0x0016);
    ADDVRIMPLICIT(0x0100);
    ADDVRIMPLICIT(0x0102);
    default:
      return false;
  }
  return true;
}

template <typename TSwap>
std::istream &FileMetaInformation::ReadCompatInternal(std::istream &is)
{
  {
    // Purposely not Re-use ReadVR since we can read VR_END
    char vr_str0[2];
    is.read(vr_str0, 2);
    if(VR::IsValid(vr_str0))
    {
      MetaInformationTS = TransferSyntax::Explicit;
      // Looks like an Explicit File Meta Information Header
      is.seekg(-6, std::ios::cur); // Seek back
      ExplicitDataElement xde;
      while(ReadExplicitDataElement<SwapperNoOp>(is, xde))
      {
        if(xde.GetVR() == VR::UN)
        {
          mdcmWarningMacro("VR::UN found in file Meta header. "
            "VR::UN will be replaced with proper VR for tag: " << xde.GetTag());
          AddVRToDataElement(xde);
        }
        Insert(xde);
      }
      // find out the dataset transfer syntax
      ComputeDataSetTransferSyntax();
    }
    else
    {
      MetaInformationTS = TransferSyntax::Implicit;
      mdcmWarningMacro("File Meta Information is implicit. VR will be explicitly added");
      // might be an implicit encoded Meta File Information header
      // GE_DLX-8-MONO2-PrivateSyntax.dcm
      is.seekg(-6, std::ios::cur); // Seek back
      ImplicitDataElement ide;
      while(ReadImplicitDataElement<SwapperNoOp>(is, ide))
      {
        if(AddVRToDataElement(ide))
        {
          Insert(ide);
        }
        else
        {
          mdcmWarningMacro("Unknown element found in Meta Header: " << ide.GetTag());
        }
      }
      // find out the dataset transfer syntax
      try
      {
        ComputeDataSetTransferSyntax();
      }
      catch(std::logic_error &)
      {
        // We were able to read some of the Meta Header, but failed to compute the DataSetTS
        // technically MDCM is able to cope with any value here. But be kind and try to have a good guess
        mdcmWarningMacro("Meta Header is bogus. Guessing DataSet TS.");
        Tag t;
        if(!t.Read<SwapperNoOp>(is))
        {
          throw std::logic_error("Cannot read very first tag");
        }
        char vr_str[3];
        is.read(vr_str, 2);
        vr_str[2] = '\0';
        VR::VRType vr = VR::GetVRType(vr_str);
        if(vr != VR::VR_END)
        {
          DataSetTS = TransferSyntax::ExplicitVRLittleEndian;
        }
        else
        {
          DataSetTS = TransferSyntax::ImplicitVRLittleEndian;
        }
        is.seekg(-6, std::ios::cur); // Seek back
      }
    }
  }
  return is;
}

void FileMetaInformation::ComputeDataSetTransferSyntax()
{
  const Tag t(0x0002,0x0010);
  const DataElement & de = GetDataElement(t);
  std::string ts;
  const ByteValue * bv = de.GetByteValue();
  if(!bv)
  {
    throw std::logic_error("Unknown Transfer syntax (!bv)");
  }
  // Pad string with a \0
  ts = std::string(bv->GetPointer(), bv->GetLength());
  TransferSyntax tst(TransferSyntax::GetTSType(ts.c_str()));
  if(tst == TransferSyntax::TS_END)
  {
    throw std::logic_error("Unknown Transfer syntax (TS_END)");
  }
  DataSetTS = tst;
#if 0
  const bool b = DataSetTS.IsValid();
  (void)b;
#endif
}

void FileMetaInformation::SetDataSetTransferSyntax(const TransferSyntax &ts)
{
  DataSetTS = ts;
}

std::string FileMetaInformation::GetMediaStorageAsString() const
{
  // D 0002|0002 [UI] [Media Storage SOP Class UID]
  // [1.2.840.10008.5.1.4.1.1.12.1]
  // ==>       [X-Ray Angiographic Image Storage]
  const Tag t(0x0002,0x0002);
  if(!FindDataElement(t))
  {
    return std::string("");
  }
  const DataElement &de = GetDataElement(t);
  std::string ts;
  {
    const ByteValue * bv = de.GetByteValue();
    if(bv && bv->GetPointer() && bv->GetLength())
    {
      // Pad string with a \0
      ts = std::string(bv->GetPointer(), bv->GetLength());
    }
  }
  const size_t ts_size = ts.size();
  if(ts_size > 0)
  {
    char & last = ts[ts_size - 1];
    if(last == ' ') last = '\0';
  }
  return ts;
}

MediaStorage FileMetaInformation::GetMediaStorage() const
{
  const std::string &ts = GetMediaStorageAsString();
  if(ts.empty()) return MediaStorage::MS_END;
  MediaStorage ms = MediaStorage::GetMSType(ts.c_str());
  if(ms == MediaStorage::MS_END)
  {
    mdcmAlwaysWarnMacro("Media Storage Class UID: " << ts << " is unknown");
  }
  return ms;
}

void FileMetaInformation::Default()
{
}

std::ostream & FileMetaInformation::Write(std::ostream &os) const
{
  P.Write(os);
  {
    this->DataSet::Write<ExplicitDataElement,SwapperNoOp>(os);
  }
  return os;
}

} // end namespace mdcm
