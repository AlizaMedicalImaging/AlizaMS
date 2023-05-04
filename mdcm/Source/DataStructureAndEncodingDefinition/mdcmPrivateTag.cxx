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

#include "mdcmPrivateTag.h"
#include "mdcmTrace.h"
#include "mdcmSystem.h"
#include <cstdio>

namespace mdcm
{

// 1234,5678,... to specify the tag (0x1234,0x5678,...)
bool
PrivateTag::ReadFromCommaSeparatedString(const char * str)
{
  if (!str)
    return false;
  unsigned int group = 0, element = 0;
  int creator_idx = -1;
  if (sscanf(str, "%04x,%04x,%n", &group , &element, &creator_idx) != 2 ||
      creator_idx == -1 || // e.g. 10 in "1234,5678,..." or 6 in "12,34,..."
      group > 65535U || element > 65535U || // this check is not required for max. width 4
      group % 2 == 0)
  {
    mdcmDebugMacro("PrivateTag::ReadFromCommaSeparatedString failed (1): " << str);
    return false;
  }
  SetGroup(static_cast<uint16_t>(group));
  SetElement(static_cast<uint8_t>(element)); // keep the lower bits of elements like 1020
  const char * creator = str + creator_idx;
  SetOwner(creator);
  const bool backslash = strchr(creator, '\\') != nullptr;
  const char * creator_test = GetOwner();
  if (!*creator_test || backslash)
  {
    mdcmDebugMacro("PrivateTag::ReadFromCommaSeparatedString failed (2): " << str);
    return false;
  }
  return true;
}

bool
PrivateTag::operator<(const PrivateTag & _val) const
{
  const Tag & t1 = *this;
  const Tag & t2 = _val;
  if (t1 == t2)
  {
    const char * s1 = Owner.c_str();
    const char * s2 = _val.GetOwner();
    assert(s1);
    assert(s2);
    if (*s1)
    {
      assert(s1[strlen(s1) - 1] != ' ');
    }
    if (*s2)
    {
      assert(s2[strlen(s2) - 1] != ' ');
    }
    bool res = strcmp(s1, s2) < 0;
#if 0
    if (*s1 && *s2 && mdcm::System::StrCaseCmp(s1, s2) == 0 && strcmp(s1, s2) != 0)
    {
      // This should only happen with e.g. "Philips MR Imaging DD 001" vs "PHILIPS MR IMAGING DD 001"
      // or "Philips Imaging DD 001" vs "PHILIPS IMAGING DD 001"
      // assert(strcmp(Owner.c_str(), _val.GetOwner()) == 0);
      // return true;
      const bool res2 = mdcm::System::StrCaseCmp(s1, s2) < 0;
      res = res2;
      assert(0);
    }
#endif
    return res;
  }
  else
  {
    return t1 < t2;
  }
}

DataElement
PrivateTag::GetAsDataElement() const
{
  DataElement de;
  de.SetTag(*this);
  de.SetVR(VR::LO);
  std::string copy = Owner;
  if (copy.size() % 2)
    copy.push_back(' ');
  de.SetByteValue(copy.c_str(), static_cast<uint32_t>(copy.size()));
  return de;
}

} // end namespace mdcm
