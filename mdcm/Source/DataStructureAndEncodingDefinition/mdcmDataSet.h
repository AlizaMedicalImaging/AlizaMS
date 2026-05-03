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
#ifndef MDCMDATASET_H
#define MDCMDATASET_H

#include "mdcmDataElement.h"
#include "mdcmTag.h"
#include "mdcmVR.h"
#include "mdcmElement.h"
#include "mdcmMediaStorage.h"
#include <set>
#include <iterator>

namespace mdcm
{
class MDCM_EXPORT DataElementException : public std::exception
{};

class PrivateTag;
/*
 * Class to represent a Data Set (which contains Data Elements)
 */
class MDCM_EXPORT DataSet
{
  friend class CSAHeader;
  friend std::ostream & operator<<(std::ostream &, const DataSet &);

public:
#if 0
  typedef std::set<DataElement> DataElementSet;
#else
  typedef std::multiset<DataElement> DataElementSet;
#endif
  typedef DataElementSet::const_iterator ConstIterator;
  typedef DataElementSet::iterator       Iterator;
  typedef DataElementSet::size_type      SizeType;
  ConstIterator
  Begin() const
  {
    return DES.cbegin();
  }
  Iterator
  Begin()
  {
    return DES.begin();
  }
  ConstIterator
  End() const
  {
    return DES.cend();
  }
  Iterator
  End()
  {
    return DES.end();
  }
  const DataElementSet &
  GetDES() const
  {
    return DES;
  }
  DataElementSet &
  GetDES()
  {
    return DES;
  }
  void
  Clear();
  SizeType
  Size() const
  {
    return DES.size();
  }
  void
  Print(std::ostream &, const std::string & indent = "") const;

  template <typename TDE>
  unsigned int
  ComputeGroupLength(const Tag & tag) const
  {
    assert(tag.GetElement() == 0x0);
    const DataElement r(tag);
    ConstIterator     it = DES.find(r);
    unsigned int      res = 0;
    for (++it; it != DES.cend() && it->GetTag().GetGroup() == tag.GetGroup(); ++it)
    {
      assert(it->GetTag().GetElement() != 0x0);
      assert(it->GetTag().GetGroup() == tag.GetGroup());
      res += it->GetLength<TDE>();
    }
    return res;
  }

  template <typename TDE>
  VL
  GetLength() const
  {
    if (DES.empty())
      return 0;
    assert(!DES.empty());
    VL ll = 0;
    assert(ll == 0);
    ConstIterator it = DES.cbegin();
    for (; it != DES.end(); ++it)
    {
      assert(!(it->GetLength<TDE>().IsUndefined()));
      if (it->GetTag() != Tag(0xfffe, 0xe00d))
      {
        ll += it->GetLength<TDE>();
      }
    }
    return ll;
  }

  void
  Insert(const DataElement &);
  void
  Replace(const DataElement &);
  void
  ReplaceEmpty(const DataElement &);

  SizeType
  Remove(const Tag & tag)
  {
    SizeType count = DES.erase(tag);
    assert(count == 0 || count == 1);
    return count;
  }

  const DataElement &
  GetDataElement(const Tag &) const;
  const DataElement &
  GetDataElement(const PrivateTag &) const;
  std::string
  GetPrivateCreator(const Tag &) const;
  bool
  FindDataElement(const Tag & t) const;
  bool
  FindDataElement(const PrivateTag &) const;
  const DataElement &
  FindNextDataElement(const Tag &) const;
  bool
  IsEmpty() const;
  MediaStorage
  GetMediaStorage() const;

  const DataElement & operator[](const Tag & t) const { return GetDataElement(t); }

  const DataElement &
  operator()(uint16_t group, uint16_t element) const
  {
    return GetDataElement(Tag(group, element));
  }

  bool operator==(const DataSet & other) const
  {
    return (DES == other.DES);
  }

  template <typename TDE, typename TSwap>
  std::istream &
  ReadNested(std::istream &);
  template <typename TDE, typename TSwap>
  std::istream &
  Read(std::istream &);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadUpToTag(std::istream &, const Tag &, const std::set<Tag> &);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadUpToTagWithLength(std::istream &, const Tag &, const std::set<Tag> &, VL &);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadSelectedTags(std::istream &, const std::set<Tag> &, bool = true);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadSelectedTagsWithLength(std::istream &, const std::set<Tag> &, VL &, bool = true);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadSelectedPrivateTags(std::istream &, const std::set<PrivateTag> & tags, bool = true);
  template <typename TDE, typename TSwap>
  std::istream &
  ReadSelectedPrivateTagsWithLength(std::istream &, const std::set<PrivateTag> & tags, VL &, bool = true);
  template <typename TDE, typename TSwap>
  const std::ostream &
  Write(std::ostream &) const;
  template <typename TDE, typename TSwap>
  std::istream &
  ReadWithLength(std::istream &, VL &);

protected:
  // This function is not safe, it does not check for the value of the tag
  void
  InsertDataElement(const DataElement &);

  // Internal function, that will compute the actual Tag (if found) of
  // a requested Private Tag (XXXX,YY,"PRIVATE")
  Tag
  ComputeDataElement(const PrivateTag &) const;

private:
  DataElementSet DES;
};

inline std::ostream &
operator<<(std::ostream & os, const DataSet & val)
{
  val.Print(os);
  return os;
}

} // end namespace mdcm

#include "mdcmDataSet.hxx"

#endif // MDCMDATASET_H
