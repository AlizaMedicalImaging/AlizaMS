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
#ifndef MDCMCSAHEADERDICT_H
#define MDCMCSAHEADERDICT_H

#include "mdcmTypes.h"
#include "mdcmTag.h"
#include "mdcmCSAHeaderDictEntry.h"
#include <iostream>
#include <iomanip>
#include <set>

namespace mdcm
{

/**
 * Class to represent a map of CSAHeaderDictEntry
 */
class MDCM_EXPORT CSAHeaderDict
{
  friend std::ostream &
  operator<<(std::ostream &, const CSAHeaderDict &);
  friend class Dicts;

public:
  typedef std::set<CSAHeaderDictEntry>          MapCSAHeaderDictEntry;
  typedef MapCSAHeaderDictEntry::iterator       Iterator;
  typedef MapCSAHeaderDictEntry::const_iterator ConstIterator;
  CSAHeaderDict()
    : CSAHeaderDictInternal()
  {
    assert(CSAHeaderDictInternal.empty());
  }
  ConstIterator
  Begin() const
  {
    return CSAHeaderDictInternal.cbegin();
  }
  ConstIterator
  End() const
  {
    return CSAHeaderDictInternal.cend();
  }
  bool
  IsEmpty() const
  {
    return CSAHeaderDictInternal.empty();
  }

  void
  AddCSAHeaderDictEntry(const CSAHeaderDictEntry & de)
  {
#ifndef NDEBUG
    MapCSAHeaderDictEntry::size_type s = CSAHeaderDictInternal.size();
#endif
    CSAHeaderDictInternal.insert(de);
    assert(s < CSAHeaderDictInternal.size());
  }

  const CSAHeaderDictEntry &
  GetCSAHeaderDictEntry(const char * name) const
  {
    MapCSAHeaderDictEntry::const_iterator it = CSAHeaderDictInternal.find(name);
    if (it != CSAHeaderDictInternal.cend())
    {
      return *it;
    }
    throw std::logic_error("Exception in GetCSAHeaderDictEntry");
  }

protected:
  void
  LoadDefault();

private:
  CSAHeaderDict &
  operator=(const CSAHeaderDict &);     // purposely not implemented
  CSAHeaderDict(const CSAHeaderDict &); // purposely not implemented
  MapCSAHeaderDictEntry CSAHeaderDictInternal;
};

inline std::ostream &
operator<<(std::ostream & os, const CSAHeaderDict & val)
{
  CSAHeaderDict::MapCSAHeaderDictEntry::const_iterator it = val.CSAHeaderDictInternal.cbegin();
  for (; it != val.CSAHeaderDictInternal.cend(); ++it)
  {
    const CSAHeaderDictEntry & de = *it;
    os << de << '\n';
  }
  return os;
}

} // end namespace mdcm

#endif // MDCMCSAHEADERDICT_H
