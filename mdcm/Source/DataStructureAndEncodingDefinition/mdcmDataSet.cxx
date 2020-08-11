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

namespace mdcm_ns
{
DataElement DataSet::DEEnd = DataElement(Tag(0xffff,0xffff));

const DataElement & DataSet::GetDEEnd() const
{
  return DEEnd;
}

std::string DataSet::GetPrivateCreator(const Tag & t) const
{
  if(t.IsPrivate() && !t.IsPrivateCreator())
  {
    Tag pc = t.GetPrivateCreator();
    if(pc.GetElement())
    {
      const DataElement r(pc);
      ConstIterator it = DES.find(r);
      if(it == DES.end()) return "";
      const DataElement & de = *it;
      if(de.IsEmpty()) return "";
      const ByteValue * bv = de.GetByteValue();
      if (!bv) return "";
      std::string owner = std::string(bv->GetPointer(),bv->GetLength());
      while(owner.size() && owner[owner.size()-1] == ' ')
      {
        owner.erase(owner.size()-1,1);
      }
      assert(owner.size() == 0 || owner[owner.size()-1] != ' ');
      return owner;
    }
  }
  return "";
}

Tag DataSet::ComputeDataElement(const PrivateTag & t) const
{
  mdcmDebugMacro( "Entering ComputeDataElement" );
  // First private creator (0x0 -> 0x9 are reserved...)
  const Tag start(t.GetGroup(), 0x0010); 
  const DataElement r(start);
  ConstIterator it = DES.lower_bound(r);
  const char *refowner = t.GetOwner();
  assert( refowner );
  bool found = false;
  while(it != DES.end() && it->GetTag().GetGroup() == t.GetGroup() && it->GetTag().GetElement() < 0x100)
  {
    const ByteValue * bv = it->GetByteValue();
    if(bv)
    {
      std::string tmp(bv->GetPointer(),bv->GetLength());
      // trim trailing whitespaces
      tmp.erase(tmp.find_last_not_of(' ') + 1);
      assert(tmp.size() == 0 || tmp[ tmp.size() - 1 ] != ' '); // FIXME
      if(System::StrCaseCmp(tmp.c_str(), refowner) == 0)
      {
        found = true;
        break;
      }
    }
    ++it;
  }
  mdcmDebugMacro("In compute found is:" << found);
  if (!found) return GetDEEnd().GetTag();
  // we found the Private Creator, let's construct the proper data element
  Tag copy = t;
  copy.SetPrivateCreator(it->GetTag());
  mdcmDebugMacro("Compute found:" << copy);
  return copy;
}

bool DataSet::FindDataElement(const PrivateTag & t) const
{
  return FindDataElement(ComputeDataElement(t));
}

const DataElement & DataSet::GetDataElement(const PrivateTag & t) const
{
  return GetDataElement(ComputeDataElement(t));
}

MediaStorage DataSet::GetMediaStorage() const
{
  const Tag tsopclassuid(0x0008, 0x0016);
  if(!FindDataElement(tsopclassuid))
  {
    mdcmDebugMacro("No SOP Class UID");
    return MediaStorage::MS_END;
  }
  const DataElement & de = GetDataElement(tsopclassuid);
  if(de.IsEmpty())
  {
    mdcmDebugMacro("Empty SOP Class UID");
    return MediaStorage::MS_END;
  }
  std::string ts;
  {
    const ByteValue * bv = de.GetByteValue();
    if(bv && bv->GetPointer() && bv->GetLength())
    {
      ts = std::string(bv->GetPointer(), bv->GetLength());
    }
    else
    {
      mdcmDebugMacro("!bv");
      return MediaStorage::MS_END;
    }
  }
  // if last character of a VR=UI is space let's pretend this is a \0
  if(ts.size())
  {
    char & last = ts[ts.size()-1];
    if(last == ' ') last = '\0';
  }
  MediaStorage ms = MediaStorage::GetMSType(ts.c_str());
  if(ms == MediaStorage::MS_END)
  {
    mdcmWarningMacro("Media Storage Class UID: " << ts << " is unknown");
  }
  return ms;
}

} // end namespace mdcm_ns
