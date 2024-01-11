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
#include "mdcmDicts.h"

namespace mdcm
{

static const DictEntry ggl("Generic Group Length", "GenericGroupLength", VR::UL, VM::VM1, true);
static const DictEntry ill("Illegal Element", "IllegalElement", VR::INVALID, VM::VM0, false);
static const DictEntry prc("Private Creator", "PrivateCreator", VR::LO, VM::VM1, false);
static const DictEntry pe1("Private Element With Empty Private Creator",
                           "PrivateElementWithEmptyPrivateCreator",
                           VR::INVALID,
                           VM::VM0,
                           false);

const DictEntry &
Dicts::GetDictEntry(const Tag & tag, const char * owner) const
{
  if (tag.IsGroupLength())
  {
    const DictEntry & de = PublicDict.GetDictEntry(tag);
    const char *      name = de.GetName();
    if (name && *name)
    {
      return de;
    }
    return ggl;
  }
  else if (tag.IsPublic())
  {
    return PublicDict.GetDictEntry(tag);
  }
  else
  {
    if (owner && *owner)
    {
      const DictEntry & de = GetPrivateDict().GetDictEntry(
        PrivateTag(tag.GetGroup(), static_cast<uint16_t>((static_cast<uint16_t>(tag.GetElement() << 8)) >> 8), owner));
      return de;
    }
    else
    {
      // 0x0000 and [0x1,0xFF] are special cases
      if (tag.IsIllegal())
      {
        return ill;
      }
      else if (tag.IsPrivateCreator())
      {
        assert(!tag.IsIllegal());
        assert(tag.GetElement());
        assert(tag.IsPrivate());
        assert(owner == nullptr);
        return prc;
      }
      else
      {
        return pe1;
      }
    }
  }
}

const DictEntry &
Dicts::GetDictEntry(const PrivateTag & tag) const
{
  return GetDictEntry(tag, tag.GetOwner());
}

const char * Dicts::GetConstructorString(ConstructorType)
{
  return "";
}

const Dict &
Dicts::GetPublicDict() const
{
  return PublicDict;
}

const PrivateDict &
Dicts::GetPrivateDict() const
{
  return ShadowDict;
}

PrivateDict &
Dicts::GetPrivateDict()
{
  return ShadowDict;
}

const CSAHeaderDict &
Dicts::GetCSAHeaderDict() const
{
  return CSADict;
}

bool
Dicts::IsEmpty() const
{
  return GetPublicDict().IsEmpty();
}

void
Dicts::LoadDefaults()
{
  PublicDict.LoadDefault();
  ShadowDict.LoadDefault();
  CSADict.LoadDefault();
}

} // end namespace mdcm
