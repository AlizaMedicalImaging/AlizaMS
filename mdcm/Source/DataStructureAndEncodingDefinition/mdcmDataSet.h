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
class MDCM_EXPORT DataElementException : public std::exception {};

class PrivateTag;
/**
 * Class to represent a Data Set (which contains Data Elements)
 * A Data Set represents an instance of a real world Information Object
 *
 * DATA SET:
 * Exchanged information consisting of a structured set of Attribute values
 * directly or indirectly related to Information Objects. The value of each
 * Attribute in a Data Set is expressed as a Data Element.
 * A collection of Data Elements ordered by increasing Data Element Tag
 * number that is an encoding of the values of Attributes of a real world
 * object.
 *
 * Implementation note. If one do:
 * DataSet ds;
 * ds.SetLength(0);
 * ds.Read(is);
 * setting length to 0 actually means try to read is as if it was a root
 * DataSet. Other value are undefined (nested dataset with undefined length)
 * or defined length (different from 0) means nested dataset with defined
 * length.
 *
 * a DataSet does not have a Transfer Syntax type, only a File does.
 */
class MDCM_EXPORT DataSet
{
  friend class CSAHeader;
public:
#if 0
  typedef std::set<DataElement> DataElementSet;
#else
  typedef std::multiset<DataElement> DataElementSet;
#endif
  typedef DataElementSet::const_iterator ConstIterator;
  typedef DataElementSet::iterator Iterator;
  typedef DataElementSet::size_type SizeType;
  ConstIterator Begin() const { return DES.begin(); }
  Iterator Begin() { return DES.begin(); }
  ConstIterator End() const { return DES.end(); }
  Iterator End() { return DES.end(); }
  const DataElementSet & GetDES() const { return DES; }
  DataElementSet & GetDES() { return DES; }

  void Clear()
  {
    DES.clear();
    assert(DES.empty());
  }

  SizeType Size() const
  {
    return DES.size();
  }

  void Print(std::ostream &os, std::string const &indent = "") const
  {
    ConstIterator it = DES.begin();
    for(; it != DES.end(); ++it)
    {
      os << indent << *it << "\n";
    }
  }

  template <typename TDE>
  unsigned int ComputeGroupLength(Tag const &tag) const
  {
    assert( tag.GetElement() == 0x0 );
    const DataElement r(tag);
    ConstIterator it = DES.find(r);
    unsigned int res = 0;
    for(++it; it != DES.end() && it->GetTag().GetGroup() == tag.GetGroup(); ++it)
    {
      assert(it->GetTag().GetElement() != 0x0);
      assert(it->GetTag().GetGroup() == tag.GetGroup());
      res += it->GetLength<TDE>();
    }
    return res;
  }

  template <typename TDE>
  VL GetLength() const
  {
    if(DES.empty()) return 0;
    assert( !DES.empty() );
    VL ll = 0;
    assert( ll == 0 );
    ConstIterator it = DES.begin();
    for( ; it != DES.end(); ++it)
    {
      assert( !(it->GetLength<TDE>().IsUndefined()) );
      if ( it->GetTag() != Tag(0xfffe,0xe00d) )
      {
        ll += it->GetLength<TDE>();
      }
    }
    return ll;
  }

  // Tag need to be >= 0x8 to be considered valid data element
  void Insert(const DataElement& de)
  {
    // FIXME: there is a special case where a dataset can have value < 0x8, see:
    // $ mdcmdump --csa mdcmData/SIEMENS-JPEG-CorruptFrag.dcm
    if(de.GetTag().GetGroup() >= 0x0008 || de.GetTag().GetGroup() == 0x4)
    {
      // prevent user error
      if(de.GetTag() == Tag(0xfffe,0xe00d)
      || de.GetTag() == Tag(0xfffe,0xe0dd)
      || de.GetTag() == Tag(0xfffe,0xe000))
      {
        ;;
      }
      else
      {
        InsertDataElement(de);
      }
    }
    else
    {
      mdcmErrorMacro( "Cannot add element with group < 0x0008 and != 0x4 in the dataset: " << de.GetTag() );
    }
  }

  void Replace(const DataElement& de)
  {
    ConstIterator it = DES.find(de);
	if(it != DES.end())
	{
	  // detect loop
	  mdcmAssertAlwaysMacro(&*it != &de);
	  DES.erase(it);
	}
    DES.insert(de);
  }

  void ReplaceEmpty(const DataElement & de)
  {
    ConstIterator it = DES.find(de);
    if(it != DES.end() && it->IsEmpty())
    {
      // detect loop
	  mdcmAssertAlwaysMacro(&*it != &de);
	  DES.erase(it);
    }
	DES.insert(de);
  }

  SizeType Remove(const Tag & tag)
  {
    DataElementSet::size_type count = DES.erase(tag);
    assert(count == 0 || count == 1);
    return count;
  }

  // Return the DataElement with Tag 't'
  // This only search at the 'root level' of the DataSet
  const DataElement & GetDataElement(const Tag & t) const
  {
    const DataElement r(t);
    ConstIterator it = DES.find(r);
    if(it != DES.end()) return *it;
    return GetDEEnd();
  }
  const DataElement& operator[] (const Tag & t) const { return GetDataElement(t); }
  const DataElement& operator() (uint16_t group, uint16_t element) const { return GetDataElement(Tag(group,element)); }

  std::string GetPrivateCreator(const Tag &) const;

  bool FindDataElement(const PrivateTag &) const;

  const DataElement& GetDataElement(const PrivateTag &) const;

  // this only search within the level of the current DataSet
  bool FindDataElement(const Tag & t) const
  {
    const DataElement r(t);
    if(DES.find(r) != DES.end()) return true;
    return false;
  }

  // WARNING: This only search at the same level as the DataSet is!
  const DataElement& FindNextDataElement(const Tag & t) const
  {
    const DataElement r(t);
    ConstIterator it = DES.lower_bound(r);
    if(it != DES.end()) return *it;
    return GetDEEnd();
  }

  bool IsEmpty() const { return DES.empty(); };

  DataSet & operator=(DataSet const & val)
  {
    DES = val.DES;
    return *this;
  }

  template <typename TDE, typename TSwap>
  std::istream & ReadNested(std::istream &);

  template <typename TDE, typename TSwap>
  std::istream & Read(std::istream &);

  template <typename TDE, typename TSwap>
  std::istream & ReadUpToTag(std::istream &, const Tag &, std::set<Tag> const &);

  template <typename TDE, typename TSwap>
  std::istream & ReadUpToTagWithLength(std::istream &, const Tag &, std::set<Tag> const &, VL &);

  template <typename TDE, typename TSwap>
  std::istream & ReadSelectedTags(std::istream &, const std::set<Tag> &, bool=true);
  template <typename TDE, typename TSwap>
  std::istream & ReadSelectedTagsWithLength(std::istream &, const std::set<Tag> &, VL &, bool=true);

  template <typename TDE, typename TSwap>
  std::istream & ReadSelectedPrivateTags(std::istream &, const std::set<PrivateTag> & tags, bool=true);
  template <typename TDE, typename TSwap>
  std::istream & ReadSelectedPrivateTagsWithLength(std::istream &, const std::set<PrivateTag> & tags, VL &, bool=true);

  template <typename TDE, typename TSwap>
  std::ostream const & Write(std::ostream &) const;

  template <typename TDE, typename TSwap>
  std::istream & ReadWithLength(std::istream &, VL &);

  MediaStorage GetMediaStorage() const;

protected:
  /* GetDEEnd is a Win32 only issue, one cannot use a dllexported
   * static member data in an inline function, otherwise symbol
   * will get reported as missing in any dll using the inlined function
   */
  const DataElement& GetDEEnd() const;

  // This function is not safe, it does not check for the value of the tag
  // so depending whether we are getting called from a dataset or file meta header
  // the condition is different
  void InsertDataElement(const DataElement & de)
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

protected:
  // Internal function, that will compute the actual Tag (if found) of
  // a requested Private Tag (XXXX,YY,"PRIVATE")
  Tag ComputeDataElement(const PrivateTag &) const;

private:
  DataElementSet DES;
  static DataElement DEEnd;
  friend std::ostream& operator<<(std::ostream &, const DataSet &);
};

inline std::ostream& operator<<(std::ostream & os, const DataSet & val)
{
  val.Print(os);
  return os;
}

#if defined(SWIGPYTHON) || defined(SWIGCSHARP) || defined(SWIGJAVA) || defined(SWIGPHP)
/*
 * HACK: I need this temp class to be able to manipulate a std::set from python,
 * swig does not support wrapping of simple class like std::set...
 */
class SWIGDataSet
{
public:
  SWIGDataSet(DataSet & des) : Internal(des), it(des.Begin()) {}
  const DataElement& GetCurrent() const { return *it; }
  void Start() { it = Internal.Begin(); }
  bool IsAtEnd() const { return it == Internal.End(); }
  void Next() { ++it; }
private:
  DataSet & Internal;
  DataSet::ConstIterator it;
};
#endif

} // end namespace mdcm

#include "mdcmDataSet.txx"

#endif //MDCMDATASET_H
