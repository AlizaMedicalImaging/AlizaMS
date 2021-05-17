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
#ifndef MDCMSCANNER_H
#define MDCMSCANNER_H

#include "mdcmSubject.h"
#include "mdcmTag.h"
#include "mdcmFile.h"
#include "mdcmDataElement.h"
#include "mdcmDataSet.h"
#include "mdcmDict.h"
#include "mdcmSmartPointer.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstring>

namespace mdcm
{

/**
 * Scanner
 *
 * This filter is dealing with both VRASCII and VRBINARY element.
 *
 * IMPORTANT In case of file where tags are not ordered (illegal as
 * per DICOM specification), the output will be missing information.
 *
 */
class MDCM_EXPORT Scanner : public Subject
{
public:
  // key comparison
  struct ltstr
  {
    bool
    operator()(const char * s1, const char * s2) const
    {
      if (s1 && s2)
        return (strcmp(s1, s2) < 0);
      return false;
    }
  };

  Scanner();
  ~Scanner();
  std::string
  GetString(const DataElement &, const DataSet &, const bool, const Dict &) const;
  // struct to map a filename to a value
  // Implementation note:
  // all std::map in this class will be using const char * and
  // not std::string since we are pointing to existing std::string
  // (hold in a std::vector), this avoid an extra copy of the byte array.
  // Tag are used as Tag class since sizeof(tag) <= sizeof(pointer)
  typedef std::map<Tag, const char *>               TagToValue;
  typedef std::map<const char *, TagToValue, ltstr> MappingType;
  typedef MappingType::const_iterator               ConstIterator;
  typedef std::set<std::string>                     ValuesType;
  typedef std::set<Tag>                             TagsType;
  void
  AddTag(const Tag &);
  void
  ClearTags();
  bool
  Scan(const std::vector<std::string> &, const Dict &);
  const std::vector<std::string> &
  GetFilenames() const;
  bool
  IsKey(const char *) const;
  std::vector<std::string>
  GetKeys() const;
  const char *
  GetValue(const char *, Tag const &) const;
  const ValuesType &
  GetValues() const;
  ValuesType
  GetValues(const Tag &) const;
  std::vector<std::string>
  GetOrderedValues(const Tag &) const;
  ConstIterator
  Begin() const
  {
    return Mappings.cbegin();
  }
  ConstIterator
  End() const
  {
    return Mappings.cend();
  }
  const MappingType &
  GetMappings() const
  {
    return Mappings;
  }
  const TagToValue &
  GetMapping(const char *) const;
  const char *
  GetFilenameFromTagToValue(const Tag &, const char *) const;
  std::vector<std::string>
  GetAllFilenamesFromTagToValue(const Tag &, const char *) const;
  const TagToValue &
  GetMappingFromTagToValue(const Tag &, const char *) const;
  static SmartPointer<Scanner>
  New()
  {
    return new Scanner;
  }
  void
  Print(std::ostream &) const override;

protected:
  void
  ProcessPublicTag(const char *, const File &, const Dict &);

private:
  std::set<Tag>            Tags;
  ValuesType               Values;
  std::vector<std::string> Filenames;
  MappingType              Mappings;
  double                   Progress;
};

} // end namespace mdcm

#endif // MDCMSCANNER_H
