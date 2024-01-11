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
#include "mdcmTag.h"
#include "mdcmTrace.h"
#include <cstdio>

namespace mdcm
{

Tag::Tag(uint16_t group, uint16_t element)
{
  ElementTag.tags[0] = group;
  ElementTag.tags[1] = element;
}

Tag::Tag(uint32_t tag)
{
  SetElementTag(tag);
}

uint16_t
Tag::GetGroup() const
{
  return ElementTag.tags[0];
}

uint16_t
Tag::GetElement() const
{
  return ElementTag.tags[1];
}

void
Tag::SetGroup(uint16_t group)
{
  ElementTag.tags[0] = group;
}

void
Tag::SetElement(uint16_t element)
{
  ElementTag.tags[1] = element;
}

void
Tag::SetElementTag(uint16_t group, uint16_t element)
{
  ElementTag.tags[0] = group;
  ElementTag.tags[1] = element;
}

uint32_t
Tag::GetElementTag() const
{
#ifndef MDCM_WORDS_BIGENDIAN
  return (ElementTag.tag << 16) | (ElementTag.tag >> 16);
#else
  return ElementTag.tag;
#endif
}

void
Tag::SetElementTag(uint32_t tag)
{
#ifndef MDCM_WORDS_BIGENDIAN
  tag = ((tag << 16) | (tag >> 16));
#endif
  ElementTag.tag = tag;
}

uint32_t
Tag::GetLength() const
{
  return 4;
}

// Does not prove the element is indeed in the dict
bool
Tag::IsPublic() const
{
  return !(ElementTag.tags[0] % 2);
}

bool
Tag::IsPrivate() const
{
  return !IsPublic();
}

Tag
Tag::GetPrivateCreator() const
{
  // See PS 3.5 - 7.8.1 PRIVATE DATA ELEMENT TAGS
  // eg: 0x0123,0x1425 -> 0x0123,0x0014
  if (IsPrivate() && !IsPrivateCreator())
  {
    Tag r = *this;
    r.SetElement(static_cast<uint16_t>(GetElement() >> 8));
    return r;
  }
  if (IsPrivateCreator())
    return *this;
  return Tag(0x0, 0x0);
}

void
Tag::SetPrivateCreator(Tag const & t)
{
  // See PS 3.5 - 7.8.1 PRIVATE DATA ELEMENT TAGS
  // eg: 0x0123,0x0045 -> 0x0123,0x4567
  assert(t.IsPrivate());
  const uint16_t element = static_cast<uint16_t>(t.GetElement() << 8);
  const uint16_t base = static_cast<uint16_t>(GetElement() << 8);
  SetElement(static_cast<uint16_t>((base >> 8) + element));
  SetGroup(t.GetGroup());
}

// Returns if tag is a Private Creator (xxxx,00yy),
// where xxxx is odd number and yy in [0x10,0xFF]
bool
Tag::IsPrivateCreator() const
{
  return IsPrivate() && (GetElement() <= 0xFF && GetElement() >= 0x10);
}

bool
Tag::IsIllegal() const
{
  // DICOM reserved those groups
  return (GetGroup() == 0x0001 || GetGroup() == 0x0003 || GetGroup() == 0x0005 || GetGroup() == 0x0007 ||
          (IsPrivate() && GetElement() > 0x0 && GetElement() < 0x10));
}

bool
Tag::IsGroupLength() const
{
  return GetElement() == 0x0;
}

// E.g 6002,3000 belong to groupXX: 6000,3000
bool
Tag::IsGroupXX(const Tag & t) const
{
  if (t.GetElement() == GetElement())
  {
    if (t.IsPrivate())
      return false;
    uint16_t group = static_cast<uint16_t>((GetGroup() >> 8) << 8);
    return group == t.GetGroup();
  }
  return false;
}

// Read from a comma separated string.
// This is a highly user oriented function, the string should be
// formated as: 1234,5678 to specify the tag (0x1234,0x5678)
// The notation comes from the DICOM standard, and is handy to use
// from a command line program
bool
Tag::ReadFromCommaSeparatedString(const char * str)
{
  unsigned int group = 0, element = 0;
  if (!str || sscanf(str, "%04x,%04x", &group, &element) != 2)
  {
    mdcmDebugMacro("Problem reading Tag: " << str);
    return false;
  }
  SetGroup(static_cast<uint16_t>(group));
  SetElement(static_cast<uint16_t>(element));
  return true;
}

// Read From XML formatted tag value eg. tag = "12345678"
// It comes in useful when reading tag values from XML file
// (in NativeDICOMModel)
bool
Tag::ReadFromContinuousString(const char * str)
{
  unsigned int group = 0, element = 0;
  if (!str || sscanf(str, "%04x%04x", &group, &element) != 2)
  {
    mdcmDebugMacro("Problem reading Tag: " << str);
    return false;
  }
  SetGroup(static_cast<uint16_t>(group));
  SetElement(static_cast<uint16_t>(element));
  return true;
}

// Read from a pipe separated string (MDCM 1.x compat only).
// Do not use in newer code
bool
Tag::ReadFromPipeSeparatedString(const char * str)
{
  unsigned int group = 0, element = 0;
  if (!str || sscanf(str, "%04x|%04x", &group, &element) != 2)
  {
    mdcmDebugMacro("Problem reading Tag: " << str);
    return false;
  }
  SetGroup(static_cast<uint16_t>(group));
  SetElement(static_cast<uint16_t>(element));
  return true;
}

// Print tag value with no separating comma: eg. tag = "12345678"
// It comes in useful when reading tag values from XML file
// (in NativeDICOMModel)
std::string
Tag::PrintAsContinuousString() const
{
  std::ostringstream os;
  const Tag &        val = *this;
  os.setf(std::ios::right);
  os << std::hex << std::setw(4) << std::setfill('0') << val[0] << std::setw(4) << std::setfill('0') << val[1]
     << std::setfill(' ') << std::dec;
  return os.str();
}

// Same as PrintAsContinuousString, but hexadecimal [a-f]
// are printed using upper case
std::string
Tag::PrintAsContinuousUpperCaseString() const
{
  std::ostringstream os;
  const Tag &        val = *this;
  os.setf(std::ios::right);
  os << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << val[0] << std::setw(4) << std::setfill('0')
     << val[1] << std::setfill(' ') << std::dec;
  return os.str();
}

// Print as a pipe separated string (MDCM 1.x compat only).
// Do not use in newer code
std::string
Tag::PrintAsPipeSeparatedString() const
{
  std::ostringstream _os;
  const Tag &        _val = *this;
  _os.setf(std::ios::right);
  _os << std::hex << std::setw(4) << std::setfill('0') << _val[0] << '|' << std::setw(4) << std::setfill('0') << _val[1]
      << std::setfill(' ') << std::dec;
  return _os.str();
}

} // end namespace mdcm
