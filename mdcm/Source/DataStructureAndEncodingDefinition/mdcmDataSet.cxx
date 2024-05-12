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
#include "mdcmDataSet.h"
#include "mdcmPrivateTag.h"
#include <cctype>
#include <algorithm>

namespace
{

std::string to_lower(std::string s)
{
  std::transform(
    s.begin(),
    s.end(),
    s.begin(),
    [](unsigned char c)
    {
      return std::tolower(c);
    });
  return s;
}

int StrCaseCmp(const char * s1, const char * s2)
{
  const std::string str1(to_lower(std::string(s1)));
  const std::string str2(to_lower(std::string(s2)));
  if (str1 == str2)
    return 0;
  else if (str1 > str2)
    return 1;
  return -1;
}

static const mdcm::DataElement empty = mdcm::DataElement(mdcm::Tag(0xffff, 0xffff));

}

namespace mdcm
{

void
DataSet::Clear()
{
  DES.clear();
}

void
DataSet::Print(std::ostream & os, const std::string & indent) const
{
  for (ConstIterator it = DES.cbegin(); it != DES.cend(); ++it)
  {
    os << indent << *it << '\n';
  }
}

void
DataSet::Insert(const DataElement & de)
{
  // FIXME: there is a special case where a dataset can have value < 0x8, see:
  // $ gdcmdump --csa gdcmData/SIEMENS-JPEG-CorruptFrag.dcm
  if (de.GetTag().GetGroup() >= 0x0008 || de.GetTag().GetGroup() == 0x4)
  {
    // prevent user error
    if (de.GetTag() == Tag(0xfffe, 0xe00d) || de.GetTag() == Tag(0xfffe, 0xe0dd) || de.GetTag() == Tag(0xfffe, 0xe000))
    {
      ;
    }
    else
    {
      InsertDataElement(de);
    }
  }
  else
  {
    mdcmErrorMacro("Cannot add element with group < 0x0008 and != 0x4 in the dataset: " << de.GetTag());
  }
}

void
DataSet::Replace(const DataElement & de)
{
  ConstIterator it = DES.find(de);
  if (it != DES.cend())
  {
#if 0
    // detect loop
    if (!(&*it != &de)) // FIXME
    {
      mdcmAlwaysWarnMacro("DataSet::Replace: loop?");
      assert(0);
    }
#endif
    DES.erase(it);
  }
  DES.insert(de);
}

void
DataSet::ReplaceEmpty(const DataElement & de)
{
  ConstIterator it = DES.find(de);
  if (it != DES.cend() && it->IsEmpty())
  {
#if 0
    // detect loop
    if (!(&*it != &de)) // FIXME
    {
      mdcmAlwaysWarnMacro("DataSet::ReplaceEmpty: loop?");
      assert(0);
    }
#endif
    DES.erase(it);
  }
  DES.insert(de);
}

const DataElement &
DataSet::GetDataElement(const Tag & t) const
{
  const DataElement r(t);
  ConstIterator     it = DES.find(r);
  if (it != DES.cend())
    return *it;
  return empty;
}

const DataElement &
DataSet::GetDataElement(const PrivateTag & t) const
{
  return GetDataElement(ComputeDataElement(t));
}

std::string
DataSet::GetPrivateCreator(const Tag & t) const
{
  if (t.IsPrivate() && !t.IsPrivateCreator())
  {
    Tag pc = t.GetPrivateCreator();
    if (pc.GetElement())
    {
      const DataElement r(pc);
      ConstIterator     it = DES.find(r);
      if (it == DES.cend())
        return std::string("");
      const DataElement & de = *it;
      if (de.IsEmpty())
        return std::string("");
      const ByteValue * bv = de.GetByteValue();
      if (!bv)
        return std::string("");
      std::string owner = std::string(bv->GetPointer(), bv->GetLength());
      while (!owner.empty() && owner[owner.size() - 1] == ' ')
      {
        owner.erase(owner.size() - 1, 1);
      }
      assert(owner.empty() || owner[owner.size() - 1] != ' ');
      return owner;
    }
  }
  return std::string("");
}

bool
DataSet::FindDataElement(const Tag & t) const
{
  return (GetDataElement(t) != empty);
}

bool
DataSet::FindDataElement(const PrivateTag & t) const
{
  return FindDataElement(ComputeDataElement(t));
}

const DataElement &
DataSet::FindNextDataElement(const Tag & t) const
{
  const DataElement r(t);
  ConstIterator     it = DES.lower_bound(r);
  if (it != DES.cend())
    return *it;
  return empty;
}

bool
DataSet::IsEmpty() const
{
  return DES.empty();
}

MediaStorage
DataSet::GetMediaStorage() const
{
  const Tag tsopclassuid(0x0008, 0x0016);
  const DataElement & de = GetDataElement(tsopclassuid);
  if (de.IsEmpty())
  {
    mdcmDebugMacro("No or empty SOP Class UID");
    return MediaStorage::MS_END;
  }
  std::string ts;
  {
    const ByteValue * bv = de.GetByteValue();
    if (bv && bv->GetPointer() && bv->GetLength())
    {
      ts = std::string(bv->GetPointer(), bv->GetLength());
    }
    else
    {
      mdcmDebugMacro("!bv");
      return MediaStorage::MS_END;
    }
  }
  // VR UI, if the last character of a ' ', replace with null
  if (ts.size())
  {
    char & last = ts[ts.size() - 1];
    if (last == ' ')
      last = '\0';
  }
  MediaStorage ms = MediaStorage::GetMSType(ts.c_str());
  if (ms == MediaStorage::MS_END)
  {
    mdcmWarningMacro("Media Storage Class UID: " << ts << " is unknown");
  }
  return ms;
}

void
DataSet::InsertDataElement(const DataElement & de)
{
#if 0
  std::pair<Iterator, bool> pr = DES.insert(de);
  if(pr.second == false)
  {
    mdcmAlwaysWarnMacro(
      "DataElement: " << de << " was already found, skipping duplicate entry.\n"
      "Original entry kept is: " << *pr.first);
  }
#else
  DES.insert(de);
#endif
  assert(de.IsEmpty() || de.GetVL() == de.GetValue().GetLength());
}

Tag
DataSet::ComputeDataElement(const PrivateTag & t) const
{
#if 0
  mdcmDebugMacro("ComputeDataElement, tag " << t);
#endif
  // First private creator (0x0 -> 0x9 are reserved...)
  const Tag         start(t.GetGroup(), 0x0010);
  const DataElement r(start);
  ConstIterator     it = DES.lower_bound(r);
  const char *      refowner = t.GetOwner();
  assert(refowner);
  bool found = false;
  while (it != DES.cend() && it->GetTag().GetGroup() == t.GetGroup() && it->GetTag().GetElement() < 0x100)
  {
    const ByteValue * bv = it->GetByteValue();
    if (bv)
    {
      std::string tmp(bv->GetPointer(), bv->GetLength());
      // trim trailing whitespaces
      tmp.erase(tmp.find_last_not_of(' ') + 1);
      assert(tmp.size() == 0 || tmp[tmp.size() - 1] != ' ');
      if (StrCaseCmp(tmp.c_str(), refowner) == 0)
      {
        found = true;
        break;
      }
    }
    ++it;
  }
#if 0
  mdcmDebugMacro("In ComputeDataElement found = " << found);
#endif
  if (!found)
    return Tag(0xffff, 0xffff);
  // we found the Private Creator, let's construct the proper data element
  Tag copy = t;
  copy.SetPrivateCreator(it->GetTag());
#if 0
  mdcmDebugMacro("In ComputeDataElement copy = " << copy);
#endif
  return copy;
}

} // end namespace mdcm
