/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmOverlay.h"
#include "mdcmTag.h"
#include "mdcmDataElement.h"
#include "mdcmDataSet.h"
#include "mdcmAttribute.h"

#include <vector>

namespace mdcm
{

class OverlayInternal
{
public:
  OverlayInternal():
  InPixelData(false),
  Group(0), // invalid default
  Rows(0),
  Columns(0),
  NumberOfFrames(0),
  Description(),
  Type(),
  FrameOrigin(0),
  BitsAllocated(0),
  BitPosition(0),
  Data() { Origin[0] = Origin[1] = 0; }
  bool InPixelData;
  unsigned short Group;
  unsigned short Rows;           // (6000,0010) US
  unsigned short Columns;        // (6000,0011) US
  unsigned int   NumberOfFrames; // (6000,0015) IS
  std::string    Description;    // (6000,0022) LO
  std::string    Type;           // (6000,0040) CS
  signed short   Origin[2];      // (6000,0050) SS
  unsigned short FrameOrigin;    // (6000,0051) US
  unsigned short BitsAllocated;  // (6000,0100) US
  unsigned short BitPosition;    // (6000,0102) US
  std::vector<char> Data; // hold the Overlay data, but not the trailing DICOM padding (\0)
  void Print(std::ostream &os) const
{
    os << "Group           0x" <<  std::hex << Group << std::dec << std::endl;
    os << "Rows            " <<  Rows << std::endl;
    os << "Columns         " <<  Columns << std::endl;
    os << "NumberOfFrames  " <<  NumberOfFrames << std::endl;
    os << "Description     " <<  Description << std::endl;
    os << "Type            " <<  Type << std::endl;
    os << "Origin[2]       " <<  Origin[0] << "," << Origin[1] << std::endl;
    os << "FrameOrigin     " <<  FrameOrigin << std::endl;
    os << "BitsAllocated   " <<  BitsAllocated << std::endl;
    os << "BitPosition     " <<  BitPosition << std::endl;
}
};

Overlay::Overlay()
{
  Internal = new OverlayInternal;
}

Overlay::~Overlay()
{
  delete Internal;
}

Overlay::Overlay(Overlay const &ov):Object(ov)
{
  Internal = new OverlayInternal;
  // TODO copy
  *Internal = *ov.Internal;
}

Overlay & Overlay::operator=(Overlay const &ov)
{
  assert(Internal);
  *Internal = *ov.Internal;
  return *this;
}

void Overlay::Update(const DataElement & de)
{
/*
  8.1.2 Overlay data encoding of related data elements
    Encoded Overlay Planes always have a bit depth of 1, and are encoded
    separately from the Pixel Data in Overlay Data (60xx,3000). The following two
    Data Elements shall define the Overlay Plane structure:
    - Overlay Bits Allocated (60xx,0100)
    - Overlay Bit Position (60xx,0102)
    Notes: 1. There is no Data Element analogous to Bits Stored (0028,0101)
    since Overlay Planes always have a bit depth of 1.
    2. Restrictions on the allowed values for these Data Elements are defined
    in PS 3.3. Formerly overlay data stored in unused bits of Pixel Data
    (7FE0,0010) was described, and these attributes had meaningful values but this
    usage has been retired. See PS 3.5 2004. For overlays encoded in Overlay Data
    Element (60xx,3000), Overlay Bits Allocated (60xx,0100) is always 1 and Overlay
    Bit Position (60xx,0102) is always 0.
*/
  assert(de.GetTag().IsPublic());
  const ByteValue* bv = de.GetByteValue();
  if(!bv) return;
  assert(bv->GetPointer() && bv->GetLength());
  std::string s(bv->GetPointer(), bv->GetLength());
  // What if a \0 can be found before the end of string...
  //assert(strlen( s.c_str()) == s.size() );

  // First thing check consistency
  if(!GetGroup())
  {
    SetGroup(de.GetTag().GetGroup());
  }
  else
  {
    assert(GetGroup() == de.GetTag().GetGroup());
  }

  if(de.GetTag().GetElement() == 0x0000) // OverlayGroupLength
  {
    ;
  }
  else if(de.GetTag().GetElement() == 0x0010) // OverlayRows
  {
    Attribute<0x6000,0x0010> at;
    at.SetFromDataElement(de);
    SetRows(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0011) // OverlayColumns
  {
    Attribute<0x6000,0x0011> at;
    at.SetFromDataElement(de);
    SetColumns(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0015) // NumberOfFramesInOverlay
  {
    Attribute<0x6000,0x0015> at;
    at.SetFromDataElement(de);
    SetNumberOfFrames(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0022) // OverlayDescription
  {
    SetDescription(s.c_str());
  }
  else if(de.GetTag().GetElement() == 0x0040) // OverlayType
  {
    SetType(s.c_str());
  }
  else if(de.GetTag().GetElement() == 0x0045) // OverlaySubtype
  {
    mdcmWarningMacro("FIXME");
  }
  else if(de.GetTag().GetElement() == 0x0050) // OverlayOrigin
  {
    Attribute<0x6000,0x0050> at;
    at.SetFromDataElement(de);
    SetOrigin(at.GetValues());
  }
  else if(de.GetTag().GetElement() == 0x0051) // ImageFrameOrigin
  {
    Attribute<0x6000,0x0051> at;
    at.SetFromDataElement(de);
    SetFrameOrigin(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0060) // OverlayCompressionCode (RET)
  {
    assert(s == "NONE"); // FIXME
  }
  else if(de.GetTag().GetElement() == 0x0100) // OverlayBitsAllocated
  {
    Attribute<0x6000,0x0100> at;
    at.SetFromDataElement(de);
    // if OverlayBitsAllocated is 1 it imply OverlayData element
    // if OverlayBitsAllocated is 16 it imply Overlay in unused pixel bits
    if(at.GetValue() != 1)
    {
      mdcmWarningMacro("Unsuported OverlayBitsAllocated: " << at.GetValue());
    }
    SetBitsAllocated(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0102) // OverlayBitPosition
  {
    Attribute<0x6000,0x0102> at;
    at.SetFromDataElement(de);
    if(at.GetValue() != 0) // For old ACR when using unused bits
    {
      mdcmDebugMacro("Unsuported OverlayBitPosition: " << at.GetValue());
    }
    SetBitPosition(at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x0110) // OverlayFormat (RET)
  {
    assert(s == "RECT");
  }
  else if(de.GetTag().GetElement() == 0x0200) // OverlayLocation (RET)
  {
    Attribute<0x6000,0x0200> at;
    at.SetFromDataElement(de);
    mdcmWarningMacro("FIXME: " << at.GetValue());
  }
  else if(de.GetTag().GetElement() == 0x1301) // ROIArea
  {
    mdcmWarningMacro("FIXME");
  }
  else if(de.GetTag().GetElement() == 0x1302) // ROIMean
  {
    mdcmWarningMacro("FIXME");
  }
  else if(de.GetTag().GetElement() == 0x1303) // ROIStandardDeviation
  {
    mdcmWarningMacro("FIXME");
  }
  else if(de.GetTag().GetElement() == 0x1500) // OverlayLabel
  {
    mdcmWarningMacro("FIXME");
  }
  else if(de.GetTag().GetElement() == 0x3000) // OverlayData
  {
    SetOverlay(bv->GetPointer(), bv->GetLength());
  }
}

bool Overlay::GrabOverlayFromPixelData(DataSet const &ds)
{
  if (Internal->NumberOfFrames > 1) return false;
  const unsigned int ovlength = Internal->Rows*Internal->Columns/8;
  Internal->Data.resize(ovlength);
  if(Internal->BitsAllocated == 16)
  {
    if(!ds.FindDataElement(Tag(0x7fe0,0x0010)) )
    {
      mdcmWarningMacro("Could not find Pixel Data. Cannot extract Overlay.");
      return false;
    }
    const DataElement &pixeldata = ds.GetDataElement(Tag(0x7fe0,0x0010));
    const ByteValue *bv = pixeldata.GetByteValue();
    if(!bv)
    {
      // XA_GE_JPEG_02_with_Overlays.dcm
      mdcmWarningMacro("Could not extract overlay from encapsulated stream.");
      return false;
    }
    assert(bv);
    const char *array = bv->GetPointer();
    // SIEMENS_GBS_III-16-ACR_NEMA_1.acr is pain to support,
    // I cannot simply use the bv->GetLength I have to use the image dim:
    const unsigned int length = ovlength * 8 * 2; //bv->GetLength();
    const uint16_t *p = (const uint16_t*)(const void*)array;
    const uint16_t *end = (const uint16_t*)(const void*)(array + length);
    assert(8*ovlength == (unsigned int)(Internal->Rows*Internal->Columns));
    if(Internal->Data.empty())
    {
      mdcmWarningMacro("Internal Data is empty.");
      return false;
    }
    unsigned char * overlay = (unsigned char*)&Internal->Data[0];
    int c = 0;
    uint16_t pmask = (uint16_t)(1 << Internal->BitPosition);
    assert(length / 2 == ovlength * 8);
    while(p != end)
    {
      const uint16_t val = *p & pmask;
      assert(val == 0x0 || val == pmask);
      // 128 -> 0x80
      if(val)
      {
        overlay[ c / 8 ] |= (unsigned char)(0x1 << c%8);
      }
      else
      {
        // else overlay[ c / 8 ] is already 0
      }
      ++p;
      ++c;
    }
    assert((unsigned)c / 8 == ovlength);
  }
  else
  {
    mdcmErrorMacro("Could not grab Overlay from image. Please report.");
    return false;
  }
  return true;
}

void Overlay::SetGroup(unsigned short group) { Internal->Group = group; }
unsigned short Overlay::GetGroup() const { return Internal->Group; }

void Overlay::SetRows(unsigned short rows) { Internal->Rows = rows; }
unsigned short Overlay::GetRows() const { return Internal->Rows; }
void Overlay::SetColumns(unsigned short columns) { Internal->Columns = columns; }
unsigned short Overlay::GetColumns() const { return Internal->Columns; }
void Overlay::SetNumberOfFrames(unsigned int numberofframes) { Internal->NumberOfFrames = numberofframes; }
unsigned int Overlay::GetNumberOfFrames() const { return Internal->NumberOfFrames; }
void Overlay::SetDescription(const char* description) { if(description) Internal->Description = description; }
const char *Overlay::GetDescription() const { return Internal->Description.c_str(); }
void Overlay::SetType(const char* type) { if(type) Internal->Type = type; }
const char *Overlay::GetType() const { return Internal->Type.c_str(); }
static const char *OverlayTypeStrings[] = {
  "INVALID",
  "G ",
  "R ",
};
const char *Overlay::GetOverlayTypeAsString(OverlayType ot)
{
  return OverlayTypeStrings[ (int) ot ];
}
Overlay::OverlayType Overlay::GetOverlayTypeFromString(const char *s)
{
  static const int n = sizeof(OverlayTypeStrings) / sizeof ( *OverlayTypeStrings );
  if(s)
  {
    for(int i = 0; i < n; ++i)
    {
      if(strcmp(s, OverlayTypeStrings[i]) == 0 )
      {
        return (OverlayType)i;
      }
    }
  }
  // could not find the proper type, maybe padded with \0 ?
  if(s && strlen(s) == 1)
  {
    for(int i = 0; i < n; ++i)
    {
      if(strncmp(s, OverlayTypeStrings[i], 1) == 0 )
      {
        mdcmDebugMacro("Invalid Padding for OVerlay Type");
        return (OverlayType)i;
      }
    }
  }
  return Overlay::Invalid;
}
Overlay::OverlayType Overlay::GetTypeAsEnum() const
{
  return GetOverlayTypeFromString(GetType());
}

void Overlay::SetOrigin(const signed short origin[2])
{
  if(origin)
  {
    Internal->Origin[0] = origin[0];
    Internal->Origin[1] = origin[1];
  }
}
const signed short * Overlay::GetOrigin() const
{
  return &Internal->Origin[0];
}
void Overlay::SetFrameOrigin(unsigned short frameorigin) { Internal->FrameOrigin = frameorigin; }
unsigned short Overlay::GetFrameOrigin() const { return Internal->FrameOrigin; }
void Overlay::SetBitsAllocated(unsigned short bitsallocated) { Internal->BitsAllocated = bitsallocated; }
unsigned short Overlay::GetBitsAllocated() const { return Internal->BitsAllocated; }
void Overlay::SetBitPosition(unsigned short bitposition) { Internal->BitPosition = bitposition; }
unsigned short Overlay::GetBitPosition() const { return Internal->BitPosition; }

bool Overlay::IsEmpty() const
{
  return Internal->Data.empty();
}
bool Overlay::IsZero() const
{
  if(IsEmpty()) return false;

  std::vector<char>::const_iterator it = Internal->Data.begin();
  for(; it != Internal->Data.end(); ++it)
  {
    if(*it) return true;
  }
  return false;
}
bool Overlay::IsInPixelData() const { return Internal->InPixelData; }
void Overlay::IsInPixelData(bool b) { Internal->InPixelData = b; }

inline unsigned int compute_bit_and_dicom_padding(unsigned short rows, unsigned short columns)
{
  unsigned int word_padding = (rows * columns + 7) / 8; // need to send full word (8bits at a time)
  return word_padding + word_padding%2; // Cannot have odd length
}

void Overlay::SetOverlay(const char *array, size_t length)
{
  if(!array || !length) return;
  size_t computed_length = 0;
  if (Internal->NumberOfFrames > 0)
    computed_length = (Internal->Rows*Internal->Columns*Internal->NumberOfFrames + 7) / 8;
  else
    computed_length = (Internal->Rows*Internal->Columns + 7) / 8;
  Internal->Data.resize(computed_length); // filled with 0 if length < computed_length
  if(length < computed_length)
  {
    mdcmWarningMacro("Not enough data found in Overlay. Proceed with caution");
    std::copy(array, array+length, Internal->Data.begin());
  }
  else
  {
    // do not try to copy more than allocated:
    // technically we may be missing the trailing DICOM padding (\0),
    // but we have all the data needed anyway:
    std::copy(array, array+computed_length, Internal->Data.begin());
  }
  /* warning need to take into account padding to the next word (8bits) */
  assert(Internal->Data.size() == computed_length);
}

const ByteValue &Overlay::GetOverlayData() const
{
  static ByteValue bv;
  bv = ByteValue(Internal->Data);
  return bv;
}

size_t Overlay::GetUnpackBufferLength() const
{
  if (Internal->NumberOfFrames > 0)
  {
    return (size_t)
      (Internal->Rows*Internal->Columns*Internal->NumberOfFrames); 
  }
  return (size_t)(Internal->Rows*Internal->Columns);
}

bool Overlay::GetUnpackBuffer(char *buffer, size_t len) const
{
  const size_t unpacklen = GetUnpackBufferLength();
  if(len < unpacklen) return false;
  unsigned char *unpackedbytes = (unsigned char*)buffer;
  const unsigned char *begin = unpackedbytes;
  for(std::vector<char>::const_iterator it =
        Internal->Data.begin(); it != Internal->Data.end(); ++it)
  {
    assert(unpackedbytes <= begin + len);
    unsigned char packedbytes = static_cast<unsigned char>(*it);
    unsigned char mask = 1;
    for (unsigned int i = 0; i < 8 && unpackedbytes < begin + len; ++i)
    {
      if ((packedbytes & mask) == 0)
      {
        *unpackedbytes = 0;
      }
      else
      {
        *unpackedbytes = 255;
      }
      ++unpackedbytes;
      mask <<= 1;
    }
  }
  assert(unpackedbytes <= begin + len);
  return true;
}

void Overlay::Decompress(std::ostream &os) const
{
  const size_t unpacklen = GetUnpackBufferLength();
  unsigned char unpackedbytes[8];
  size_t curlen = 0;
  for(std::vector<char>::const_iterator it =
    Internal->Data.begin(); it != Internal->Data.end(); ++it)
  {
    unsigned char packedbytes = *it;
    unsigned char mask = 1;
    unsigned int i = 0;
    for (; i < 8 && curlen < unpacklen; ++i)
    {
      if ((packedbytes & mask) == 0)
      {
        unpackedbytes[i] = 0;
      }
      else
      {
        unpackedbytes[i] = 255;
      }
      mask <<= 1;
      ++curlen;
    }
    os.write(reinterpret_cast<char*>(unpackedbytes), i);
  }
}

void Overlay::Print(std::ostream &os) const
{
  Internal->Print(os);
}

} // end namespace mdcm
