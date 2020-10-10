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
#include "mdcmPrivateTag.h"
#include "mdcmSmartPointer.h"
#include <map>
#include <set>
#include <string>
#include <cstring>

namespace mdcm
{
class StringFilter;

/**
 * Scanner
 *
 * This filter is dealing with both VRASCII and VRBINARY element, thanks to the
 * help of StringFilter
 *
 * IMPORTANT In case of file where tags are not ordered (illegal as
 * per DICOM specification), the output will be missing information
 *
 */
class MDCM_EXPORT Scanner : public Subject
{
  friend std::ostream& operator<<(std::ostream &, const Scanner &);

public:
  Scanner() : Values(), Filenames(), Mappings(), Progress(0.0) {}
  ~Scanner();

  // struct to map a filename to a value
  // Implementation note:
  // all std::map in this class will be using const char * and not std::string
  // since we are pointing to existing std::string (hold in a std::vector)
  // this avoid an extra copy of the byte array.
  // Tag are used as Tag class since sizeof(tag) <= sizeof(pointer)
  typedef std::map<Tag, const char *> TagToValue;
  typedef TagToValue::value_type TagToValueValueType;
  void AddTag(Tag const &);
  void ClearTags();
  // Work in progress do not use
  void AddPrivateTag(PrivateTag const &);
  void AddSkipTag(Tag const &);
  void ClearSkipTags();
  bool Scan(std::vector<std::string> const &);
  std::vector<std::string> const &GetFilenames() const { return Filenames; }
  void Print(std::ostream &) const;
  bool IsKey(const char *) const;
  std::vector<std::string> GetKeys() const;
  typedef std::set<std::string> ValuesType;
  ValuesType const & GetValues() const { return Values; }
  ValuesType GetValues(Tag const &) const;
  std::vector<std::string> GetOrderedValues(Tag const &) const;
  /* ltstr is CRITICAL, otherwise pointers value are used to do the key comparison */
  struct ltstr
  {
    bool operator()(const char * s1, const char * s2) const
    {
      assert(s1 && s2);
      return strcmp(s1, s2) < 0;
    }
  };
  typedef std::map<const char *,TagToValue, ltstr> MappingType;
  typedef MappingType::const_iterator ConstIterator;
  ConstIterator Begin() const { return Mappings.begin(); }
  ConstIterator End() const { return Mappings.end(); }
  MappingType const & GetMappings() const { return Mappings; }
  TagToValue const & GetMapping(const char *filename) const;
  const char *GetFilenameFromTagToValue(Tag const &, const char *) const;
  std::vector<std::string> GetAllFilenamesFromTagToValue(Tag const &, const char *) const;
  TagToValue const & GetMappingFromTagToValue(Tag const &, const char *) const;
  // Tag 't' should have been added via AddTag() prior to the Scan() call
  const char* GetValue(const char *, Tag const &) const;
  static SmartPointer<Scanner> New() { return new Scanner; }

protected:
  void ProcessPublicTag(StringFilter &, const char *);

private:
  typedef std::set<Tag> TagsType;
  typedef std::set<PrivateTag > PrivateTagsType;
  std::set<Tag> Tags;
  std::set<PrivateTag> PrivateTags;
  std::set<Tag> SkipTags;
  ValuesType Values;
  std::vector<std::string> Filenames;
  MappingType Mappings;
  double Progress;
};

inline std::ostream& operator<<(std::ostream & os, const Scanner & s)
{
  s.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMSCANNER_H
