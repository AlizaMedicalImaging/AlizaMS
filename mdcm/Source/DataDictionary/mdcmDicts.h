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

#ifndef MDCMDICTS_H
#define MDCMDICTS_H

#include "mdcmDict.h"
#include "mdcmCSAHeaderDict.h"

namespace mdcm
{
/**
 * Class to manipulate the dictionaries
 */
class MDCM_EXPORT Dicts
{
public:
  Dicts();
  ~Dicts();
  const DictEntry & GetDictEntry(const Tag &, const char (*)= NULL) const;
  const DictEntry & GetDictEntry(const PrivateTag &) const;
  const Dict & GetPublicDict() const;
  const PrivateDict & GetPrivateDict() const;
  PrivateDict & GetPrivateDict();
  const CSAHeaderDict & GetCSAHeaderDict() const;
  bool IsEmpty() const;

protected:
  typedef enum
  {
    PHILIPS,
    GEMS,
    SIEMENS
  } ConstructorType;
  static const char * GetConstructorString(ConstructorType);
  friend class Global;
  void LoadDefaults();

private:
  Dict PublicDict;
  PrivateDict ShadowDict;
  CSAHeaderDict CSADict;
  Dicts &operator=(const Dicts &); // purposely not implemented
  Dicts(const Dicts &); // purposely not implemented
};

} // end namespace mdcm

#endif //MDCMDICTS_H
