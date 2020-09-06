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

#include "mdcmCSAHeader.h"
#include "mdcmPrivateTag.h"
#include "mdcmDataElement.h"
#include "mdcmCSAElement.h"
#include "mdcmDataSet.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmSwapper.h"

namespace mdcm
{

CSAElement CSAHeader::CSAEEnd = CSAElement((unsigned int)-1);

const CSAElement& CSAHeader::GetCSAEEnd() const
{
  return CSAEEnd;
}

struct DataSetFormatEntry
{
  Tag t;
  VR  vr;
};

static DataSetFormatEntry DataSetFormatDict[] = {
 { Tag(0x0000,0x0004),VR::LT },
 { Tag(0x0000,0x0005),VR::FD },
 { Tag(0x0000,0x0006),VR::FD },
 { Tag(0x0000,0x0007),VR::SL },
 { Tag(0x0000,0x0008),VR::SL },
 { Tag(0x0000,0x000c),VR::FD },
 { Tag(0x0000,0x000d),VR::SL },
 { Tag(0x0000,0x000e),VR::SL },
 { Tag(0x0000,0x0012),VR::FD },
 { Tag(0x0000,0x0013),VR::SL },
 { Tag(0x0000,0x0014),VR::SL },
 { Tag(0x0000,0x0016),VR::SL },
 { Tag(0x0000,0x0018),VR::SL },
 { Tag(0x0000,0x001a),VR::SL },
 { Tag(0x0000,0x001d),VR::LT }, // Defined Terms: [A.B.D.O.M.E.N], [C.H.E.S.T], [E.X.T.R.E.M.I.T.Y], [H.E.A.D], [N.E.C.K], [P.E.L.V.I.S], [S.P.I.N.E]
 { Tag(0x0000,0x001e),VR::FD },
 { Tag(0x0000,0x001f),VR::FD },
 { Tag(0x0000,0x0020),VR::FD },
 { Tag(0x0000,0x0021),VR::SL },
 { Tag(0x0000,0x0022),VR::SL },
 { Tag(0x0000,0x0025),VR::SL },
 { Tag(0x0000,0x0026),VR::FD },
 { Tag(0x0000,0x0027),VR::FD },
 { Tag(0x0000,0x0028),VR::SL },
 { Tag(0x0000,0x0029),VR::SL },
 { Tag(0x0000,0x002b),VR::LT },
 { Tag(0x0000,0x002c),VR::SL },
 { Tag(0x0000,0x002d),VR::SL },
 { Tag(0x0000,0x002e),VR::SL },
 { Tag(0x0000,0x002f),VR::FD },
 { Tag(0x0000,0x0030),VR::LT },
 { Tag(0x0000,0x0033),VR::SL },
 { Tag(0x0000,0x0035),VR::SL },
 { Tag(0x0000,0x0036),VR::CS },
 { Tag(0x0000,0x0037),VR::SL },
 { Tag(0x0000,0x0038),VR::SL },
 { Tag(0x0000,0x0039),VR::SL },
 { Tag(0x0000,0x003a),VR::FD },
 { Tag(0x0000,0x003b),VR::SL },
 { Tag(0x0000,0x003c),VR::SL },
 { Tag(0x0000,0x003d),VR::FD },
 { Tag(0x0000,0x003e),VR::SL },
 { Tag(0x0000,0x003f),VR::SL },
 { Tag(0x0000,0x0101),VR::FD },
 { Tag(0x0000,0x0102),VR::FD },
 { Tag(0x0000,0x0103),VR::FD },
 { Tag(0x0000,0x0105),VR::IS },
 { Tag(0x0006,0x0006),VR::FD },
 { Tag(0x0006,0x0007),VR::FD },
 { Tag(0x0006,0x0008),VR::CS },
 { Tag(0x0006,0x000a),VR::LT },
 { Tag(0x0006,0x000b),VR::CS },
 { Tag(0x0006,0x000c),VR::FD },
 { Tag(0x0006,0x000e),VR::CS },
 { Tag(0x0006,0x000f),VR::SL },
 { Tag(0x0006,0x0024),VR::FD },
 { Tag(0xffff,0xffff),VR::CS }
};

VR GetVRFromDataSetFormatDict(const Tag& t)
{
  static const unsigned int nentries = sizeof(DataSetFormatDict) / sizeof(*DataSetFormatDict);
  VR ret = VR::VR_END;
  //static const Tag tend = Tag(0xffff,0xffff);
  for(unsigned int i = 0; i < nentries; ++i)
  {
    const DataSetFormatEntry &entry = DataSetFormatDict[i];
    if(entry.t == t)
    {
      ret = entry.vr;
      break;
    }
  }
  return ret;
}

struct equ
{
  uint32_t syngodt;
  const char vr[2+1];
};

// Looks like there is mapping in between syngodt and vr...
//  O <=> UN
//  3 <=> DS
//  4 <=> FD
//  5 <=> FL
//  6 <=> IS
//  9 <=> UL
// 10 <=> US
// 16 <=> CS
// 19 <=> LO
// 20 <=> LT
// 22 <=> SH
// 25 <=> UI
static equ mapping[] = {
{  0 , "UN" },
{  3 , "DS" },
{  4 , "FD" },
{  5 , "FL" },
{  6 , "IS" },
{  7 , "SL" },
{  8 , "SS" },
{  9 , "UL" },
{ 10 , "US" },
{ 16 , "CS" },
{ 19 , "LO" },
{ 20 , "LT" },
{ 22 , "SH" },
{ 23 , "ST" },
{ 25 , "UI" },
{ 27 , "UT" }
};

bool check_mapping(uint32_t syngodt, const char *vr)
{
  static const unsigned int max = sizeof(mapping) / sizeof(equ);
  const equ *p = mapping;
  assert(syngodt <= mapping[max-1].syngodt); (void)max;
  while(p->syngodt < syngodt)
  {
    ++p;
  }
  assert(p->syngodt == syngodt); // or else need to update mapping
  const char* lvr = p->vr;
  int check = strcmp(vr, lvr) == 0;
  assert(check); (void)check;
  return true;
}

bool checkallzero(std::istream &is)
{
  bool res = true;
  char c;
  while(is >> c)
  {
    if(c != 0)
    {
      res = false;
      break;
    }
  }
  return res;
}

CSAHeader::CSAHeaderType CSAHeader::GetFormat() const
{
  return InternalType;
}

bool CSAHeader::LoadFromDataElement(DataElement const &de)
{
  if(de.IsEmpty()) return false;

  InternalCSADataSet.clear();
  InternalDataSet.Clear();
  mdcmDebugMacro("Entering print");
  InternalType = UNKNOWN; // reset
  mdcm::Tag t1(0x0029,0x0010);
  mdcm::Tag t2(0x0029,0x0020);
  uint16_t v = (uint16_t)(de.GetTag().GetElement() << 8);
  uint16_t v2 = (uint16_t)(v >> 8);
  if(v2 == t1.GetElement())
  {
    DataElementTag = t1;;
  }
  else if(v2 == t2.GetElement())
  {
    DataElementTag = t2;;
  }
  else
  {
    DataElementTag = de.GetTag();
  }
  mdcmDebugMacro("found type");

  const ByteValue *bv = de.GetByteValue();
  assert(bv);
  const char *p = bv->GetPointer();
  assert(p);
  std::string s(bv->GetPointer(), bv->GetLength());
  std::stringstream ss;
  ss.str(s);
  char signature[4+1];
  signature[4] = 0;
  if(!ss.read(signature, 4))
  {
    mdcmErrorMacro("Too short");
    return false;
  }
  // Some silly software consider the tag to be OW, therefore they byteswap it !!! sigh
  if(strcmp(signature, "VS01") == 0)
  {
    SwapperDoOp::SwapArray((unsigned short*)&s[0], (s.size() + 1) / 2);
    ss.str(s);
    ss.read(signature, 4);
  }
  //std::cout << signature << std::endl;
  // 1. NEW FORMAT
  // 2. OLD FORMAT
  // 3. Zero out
  // 4. DATASET FORMAT (Explicit Little Endian), with group=0x0 elements:
  if(strcmp(signature, "SV10") != 0)
  {
    if(checkallzero(ss))
    {
      InternalType = ZEROED_OUT;
      return true;
    }
    else if(strcmp(signature, "!INT") == 0)
    {
      InternalType = INTERFILE;
      InterfileData = p;
      return true;
    }
    else if(memcmp(signature, "\0\0" , 2) == 0 ||
            memcmp(signature, "\6\0" , 2) == 0)
  {
      // Most often start with an element (0000,xxxx)
      // And ends with element:
      // (ffff,ffff)  CS  10  END!
      ss.seekg(0, std::ios::beg);
      // SIEMENS-JPEG-CorruptFragClean.dcm
      InternalType = DATASET_FORMAT;
      DataSet &ds = InternalDataSet;
      DataElement xde;
      try
      {
        while(xde.Read<ExplicitDataElement,SwapperNoOp>(ss))
        {
          ds.InsertDataElement(xde); // Cannot use Insert since Group = 0x0 (< 0x8)
        }
        assert(ss.eof());
      }
      catch(std::exception &)
      {
        mdcmErrorMacro("Something went wrong while decoding... please report");
        return false;
      }
      return true;
    }
    else
    {
      ss.seekg(0, std::ios::beg);
      // No magic number for this one:
      // SIEMENS_Sonata-16-MONO2-Value_Multiplicity.dcm
      InternalType = NOMAGIC;
    }
  }
  if(strcmp(signature, "SV10") == 0)
  {
    // NEW FORMAT !
    ss.read(signature, 4);
    assert(strcmp(signature, "\4\3\2\1") == 0);
    InternalType = SV10;
  }
  assert(InternalType != UNKNOWN);
  mdcmDebugMacro("Found Type: " << (int)InternalType);

  uint32_t n;
  ss.read((char*)&n, sizeof(n));
  SwapperNoOp::SwapArray(&n,1);
  uint32_t unused;
  ss.read((char*)&unused, sizeof(unused));
  SwapperNoOp::SwapArray(&unused,1);
  if(unused != 77)
  {
    mdcmErrorMacro("Must be a new format. Giving up");
    return false;
  }
  assert(unused == 77); // 'M' character...

  for(uint32_t i = 0; i < n; ++i)
  {
    CSAElement csael;
    csael.SetKey(i);
    char name[64+1];
    name[64] = 0; // security
    ss.read(name, 64);
    csael.SetName(name);
    uint32_t vm;
    ss.read((char*)&vm, sizeof(vm));
    SwapperNoOp::SwapArray(&vm,1);
    csael.SetVM(VM::GetVMTypeFromLength(vm,1));
    char vr[4];
    ss.read(vr, 4);
    assert(vr[2] == 0);
    csael.SetVR(VR::GetVRTypeFromFile(vr));
    uint32_t syngodt;
    ss.read((char*)&syngodt, sizeof(syngodt));
    SwapperNoOp::SwapArray(&syngodt,1);
    bool cm = check_mapping(syngodt, vr);
    if(!cm)
    {
      mdcmErrorMacro("SyngoDT is not handled, please submit bug report");
      return false;
    }
    csael.SetSyngoDT(syngodt);
    uint32_t nitems;
    ss.read((char*)&nitems, sizeof(nitems));
    SwapperNoOp::SwapArray(&nitems,1);
    csael.SetNoOfItems(nitems);
    uint32_t xx;
    ss.read((char*)&xx, sizeof(xx));
    SwapperNoOp::SwapArray(&xx,1);
    assert(xx == 77 || xx == 205);
    std::ostringstream os;
    for(uint32_t j = 0; j < nitems; ++j)
    {
      uint32_t item_xx[4];
      ss.read((char*)&item_xx, 4*sizeof(uint32_t));
      SwapperNoOp::SwapArray(item_xx,4);
      assert(item_xx[2] == 77 || item_xx[2] == 205);
      uint32_t len = item_xx[1]; // 2nd element
      assert(item_xx[0] == item_xx[1] && item_xx[1] == item_xx[3]);
      if(len)
      {
        char *val = new char[len+1];
        val[len] = 0; // security
        ss.read(val,len);
        // simply print the value as if it was IS/DS or LO (ASCII)
        if(j)
        {
          os << '\\';
        }
        os << val;
        char dummy[4];
        uint32_t dummy_len = (4 - len % 4) % 4;
        ss.read(dummy, dummy_len);
        delete[] val;
      }
    }
    std::string str = os.str();
    if(!str.empty()) csael.SetByteValue(&str[0], (uint32_t)str.size());
    InternalCSADataSet.insert(csael);
  }
  return true;
}

void CSAHeader::Print(std::ostream &os) const
{
  std::set<CSAElement>::const_iterator it = InternalCSADataSet.begin();
  mdcm::Tag t1(0x0029,0x0010);
  mdcm::Tag t2(0x0029,0x0020);
  if(DataElementTag == t1)
  {
    os << "Image shadow data (0029|xx10)\n\n";
  }
  else if(DataElementTag == t2)
  {
    os << "Series shadow data (0029|xx20)\n\n";
  }
  for(; it != InternalCSADataSet.end(); ++it)
  {
    if (!((*it).IsEmpty())) os << *it << std::endl;
  }
}

const CSAElement &CSAHeader::GetCSAElementByName(const char *name)
{
  if(name)
  {
    std::set<CSAElement>::const_iterator it = InternalCSADataSet.begin();
    for(; it != InternalCSADataSet.end(); ++it)
    {
      const char *itname = it->GetName();
      assert(itname);
      if(strcmp(name, itname) == 0)
      {
        return *it;
      }
    }
  }
  return GetCSAEEnd();
}

bool CSAHeader::FindCSAElementByName(const char *name)
{
  if(name)
  {
    std::set<CSAElement>::const_iterator it = InternalCSADataSet.begin();
    for(; it != InternalCSADataSet.end(); ++it)
    {
      const char *itname = it->GetName();
      assert(itname);
      if(strcmp(name, itname) == 0)
      {
        return true;
      }
    }
  }
  return false;
}

static const char csaheader[] = "SIEMENS CSA HEADER";
static const mdcm::PrivateTag t1(0x0029,0x0010,csaheader); // CSA Image Header Info
static const mdcm::PrivateTag t2(0x0029,0x0020,csaheader); // CSA Series Header Info
static const char csanonimage[] = "SIEMENS CSA NON-IMAGE";
static const mdcm::PrivateTag t3(0x0029,0x0010,csanonimage); // CSA Data Info

const PrivateTag & CSAHeader::GetCSAImageHeaderInfoTag()
{
  return t1;
}

const PrivateTag & CSAHeader::GetCSASeriesHeaderInfoTag()
{
  return t2;
}

const PrivateTag & CSAHeader::GetCSADataInfo()
{
  return t3;
}

bool CSAHeader::GetMrProtocol(const DataSet & ds, MrProtocol & mrProtocol)
{
  if(!ds.FindDataElement(t2))
    return false;
  if(!LoadFromDataElement(ds.GetDataElement(t2)))
    return false;

  //  28 - 'MrProtocolVersion' VM 1, VR IS, SyngoDT 6, NoOfItems 6, Data '21710006'
  int mrprotocolversion = 0;
  static const char version[] = "MrProtocolVersion";
  // This is not an error if we do not find the version:
  if(FindCSAElementByName(version))
  {
    const CSAElement &csavers = GetCSAElementByName(version);
    if(!csavers.IsEmpty())
    {
      const ByteValue* bv = csavers.GetByteValue();
      std::string str(bv->GetPointer(), bv->GetLength());
      std::istringstream is(str);
      is >> mrprotocolversion;
    }
  }

  static const char * candidates[] =
  {
    "MrProtocol",
    "MrPhoenixProtocol" 
  };

  static const int n = sizeof candidates / sizeof * candidates;
  bool found = false;
  for(int i = 0; i < n; ++i)
  {
    const char * candidate = candidates[i];
    if(FindCSAElementByName(candidate))
    {
      // assume the correct one is the one that is not empty,
      // ideally we should rely on the version...
      const mdcm::CSAElement &csael = GetCSAElementByName(candidate);
      if(!csael.IsEmpty())
      {
        if(mrProtocol.Load(csael.GetByteValue(), candidate, mrprotocolversion))
        {
          found = true;
        }
      }
    }
  }
  return found;
}

} // end namespace mdcm
