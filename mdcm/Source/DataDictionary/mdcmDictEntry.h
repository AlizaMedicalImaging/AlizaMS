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
#ifndef MDCMDICTENTRY_H
#define MDCMDICTENTRY_H

#include "mdcmVR.h"
#include "mdcmVM.h"
#include <string>
#include <iostream>
#include <iomanip>

namespace mdcm
{
/**
 * Class to represent an Entry in the Dict
 */
class MDCM_EXPORT DictEntry
{
public:
  DictEntry(const char * name = "",
            const char * keyword = "",
            const VR &   vr = VR::INVALID,
            const VM &   vm = VM::VM0,
            bool         ret = false)
    : Name(name)
    , Keyword(keyword)
    , ValueRepresentation(vr)
    , ValueMultiplicity(vm)
    , Retired(ret)
    , GroupXX(false)
    , ElementXX(false)
  {}
  friend std::ostream &
  operator<<(std::ostream & _os, const DictEntry & _val);
  const VR &
  GetVR() const
  {
    return ValueRepresentation;
  }
  void
  SetVR(const VR & vr)
  {
    ValueRepresentation = vr;
  }
  const VM &
  GetVM() const
  {
    return ValueMultiplicity;
  }
  void
  SetVM(const VM & vm)
  {
    ValueMultiplicity = vm;
  }
  const char *
  GetName() const
  {
    return Name.c_str();
  }
  void
  SetName(const char * name)
  {
    Name = name;
  }
  const char *
  GetKeyword() const
  {
    return Keyword.c_str();
  }
  void
  SetKeyword(const char * keyword)
  {
    Keyword = keyword;
  }
  bool
  GetRetired() const
  {
    return Retired;
  }
  void
  SetRetired(bool retired)
  {
    Retired = retired;
  }
  void
  SetGroupXX(bool v)
  {
    GroupXX = v;
  }
  void
  SetElementXX(bool v)
  {
    ElementXX = v;
  }
  bool
  IsUnique() const
  {
    return ElementXX == false && GroupXX == false;
  }

private:
  friend class Dict;

private:
  std::string Name;
  std::string Keyword;
  VR          ValueRepresentation;
  VM          ValueMultiplicity;
  bool        Retired : 1;
  bool        GroupXX : 1;
  bool        ElementXX : 1;
};

inline std::ostream &
operator<<(std::ostream & os, const DictEntry & val)
{
  if (val.Name.empty())
  {
    os << "[No name]";
  }
  else
  {
    os << val.Name;
  }
  if (val.Keyword.empty())
  {
    os << "[No keyword]";
  }
  else
  {
    os << val.Keyword;
  }
  os << "\t" << val.ValueRepresentation << "\t" << val.ValueMultiplicity;
  if (val.Retired)
  {
    os << "\t(RET)";
  }
  return os;
}

} // end namespace mdcm

#endif // MDCMDICTENTRY_H
