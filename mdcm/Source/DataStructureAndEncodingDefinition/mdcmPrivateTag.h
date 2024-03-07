/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMPRIVATETAG_H
#define MDCMPRIVATETAG_H

#include "mdcmTag.h"
#include "mdcmVR.h"
#include "mdcmDataElement.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <cstring>
#include <ctype.h>

namespace mdcm
{

/**
 * Class to represent a Private DICOM Data Element (Attribute) Tag (Group, Element, Owner)
 * Private tag have element value in: [0x10,0xff], for instance 0x0009,0x0000 is NOT a private tag
 */

class MDCM_EXPORT PrivateTag : public Tag
{
  friend std::ostream &
  operator<<(std::ostream & _os, const PrivateTag & _val);

public:
  PrivateTag() = default;

  PrivateTag(uint16_t group, uint16_t element, const char * owner = "")
    : Tag(group, element), Owner(owner ? LOComp::Trim(owner) : "")
  {
    // Truncate the high bits.
    SetElement(static_cast<uint8_t>(element));
  }

  PrivateTag(const Tag & t, const char * owner = "")
    : Tag(t), Owner(owner ? LOComp::Trim(owner) : "")
  {
    // Truncate the high bits.
    SetElement(static_cast<uint8_t>(t.GetElement()));
  }

  const char *
  GetOwner() const
  {
    return Owner.c_str();
  }

  void
  SetOwner(const char * owner)
  {
    if (owner)
      Owner = LOComp::Trim(owner);
  }

  bool
  operator<(const PrivateTag & _val) const;
  // Element number will be truncated to 8bits.
  // Eg: "1234,5678,MDCM" is private tag: (1234,78,"MDCM")
  bool
  ReadFromCommaSeparatedString(const char * str);
  DataElement
  GetAsDataElement() const;

private:
  std::string Owner{""};
};

inline std::ostream &
operator<<(std::ostream & os, const PrivateTag & val)
{
  os.setf(std::ios::right);
  os << std::hex << '(' << std::setw(4) << std::setfill('0') << val[0]
     << ',' << std::setw(2) << std::setfill('0')
     << val[1] << ',' << val.Owner  << ')' << std::setfill(' ') << std::dec;
  return os;
}

} // end namespace mdcm

#endif // MDCMPRIVATETAG_H
