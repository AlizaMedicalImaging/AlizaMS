/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef MDCMDICTS_H
#define MDCMDICTS_H

#include "mdcmDict.h"
#include "mdcmCSAHeaderDict.h"
#include <string>

namespace mdcm
{
/**
 * \brief Class to manipulate the dict
 */
class MDCM_EXPORT Dicts
{
  friend std::ostream& operator<<(std::ostream & _os, const Dicts & d);
public:
  Dicts();
  ~Dicts();
  const DictEntry & GetDictEntry(const Tag & tag, const char * owner = NULL) const;
  const DictEntry & GetDictEntry(const PrivateTag& tag) const;
  const Dict & GetPublicDict() const;
  const PrivateDict & GetPrivateDict() const;
  PrivateDict & GetPrivateDict();
  const CSAHeaderDict & GetCSAHeaderDict() const;
  bool IsEmpty() const { return GetPublicDict().IsEmpty(); }
protected:
  typedef enum
  {
    PHILIPS,
    GEMS,
    SIEMENS
  } ConstructorType;
  static const char * GetConstructorString(ConstructorType type);
  friend class Global;
  void LoadDefaults();
private:
  Dict PublicDict;
  PrivateDict ShadowDict;
  CSAHeaderDict CSADict;
  Dicts &operator=(const Dicts & _val); // purposely not implemented
  Dicts(const Dicts & _val); // purposely not implemented
};

inline std::ostream& operator<<(std::ostream & os, const Dicts & d)
{
  (void)d;
  return os;
}


} // end namespace mdcm

#endif //MDCMDICTS_H
