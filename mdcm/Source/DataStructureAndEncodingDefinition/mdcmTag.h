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
#ifndef MDCMTAG_H
#define MDCMTAG_H

#include "mdcmTypes.h"
#include "mdcmTrace.h"
#include <iostream>
#include <iomanip>

namespace mdcm
{

/**
 * Class to represent a DICOM Data Element (Attribute) Tag (Group, Element).
 * Basically an uint32_t which can also be expressed as two uint16_t
 * (group and element).
 * DATA ELEMENT TAG:
 * A unique identifier for a Data Element composed of an ordered pair of
 * numbers (a Group Number followed by an Element Number).
 * GROUP NUMBER: The first number in the ordered pair of numbers that
 * makes up a Data Element Tag.
 * ELEMENT NUMBER: The second number in the ordered pair of numbers that
 * makes up a Data Element Tag.
 */

class MDCM_EXPORT Tag
{
  friend std::ostream &
  operator<<(std::ostream & _os, const Tag & _val);
  friend std::istream &
  operator>>(std::istream & _is, Tag & _val);

public:
  Tag() = default;

  Tag(uint16_t, uint16_t);

  Tag(uint32_t);

  // Returns the Group or Element of the given Tag: 0 / 1
  const uint16_t & operator[](const unsigned int & _id) const
  {
    assert(_id < 2);
    return ElementTag.tags[_id];
  }

  // Returns the Group or Element of the given Tag: 0 / 1
  uint16_t & operator[](const unsigned int & _id)
  {
    assert(_id < 2);
    return ElementTag.tags[_id];
  }

  bool
  operator==(const Tag & _val) const
  {
    return ElementTag.tag == _val.ElementTag.tag;
  }

  bool
  operator!=(const Tag & _val) const
  {
    return ElementTag.tag != _val.ElementTag.tag;
  }

  bool
  operator<(const Tag & _val) const
  {
#ifndef MDCM_WORDS_BIGENDIAN
    if (ElementTag.tags[0] < _val.ElementTag.tags[0])
      return true;
    if (ElementTag.tags[0] == _val.ElementTag.tags[0] && ElementTag.tags[1] < _val.ElementTag.tags[1])
      return true;
    return false;
#else
    // Plain comparison is enough.
    return (ElementTag.tag < _val.ElementTag.tag);
#endif
  }

  bool
  operator<=(const Tag & other) const
  {
    const Tag & t = *this;
    return t == other || t < other;
  }

  // Read a tag from binary representation
  template <typename TSwap>
  std::istream &
  Read(std::istream & is)
  {
    if (is.read(ElementTag.bytes, 4))
      TSwap::SwapArray(ElementTag.tags, 2);
    return is;
  }

  // Write a tag in binary rep
  template <typename TSwap>
  const std::ostream &
  Write(std::ostream & os) const
  {
    uint16_t copy[2];
    copy[0] = ElementTag.tags[0];
    copy[1] = ElementTag.tags[1];
    TSwap::SwapArray(copy, 2);
    return os.write(reinterpret_cast<char *>(&copy), 4);
  }

  uint16_t
  GetGroup() const;
  uint16_t
  GetElement() const;
  void SetGroup(uint16_t);
  void SetElement(uint16_t);
  void SetElementTag(uint16_t, uint16_t);
  uint32_t
  GetElementTag() const;
  void SetElementTag(uint32_t);
  uint32_t
  GetLength() const;
  bool
  IsPublic() const;
  bool
  IsPrivate() const;
  Tag
  GetPrivateCreator() const;
  void
  SetPrivateCreator(const Tag &);
  bool
  IsPrivateCreator() const;
  bool
  IsIllegal() const;
  bool
  IsGroupLength() const;
  bool
  IsGroupXX(const Tag &) const;
  bool
  ReadFromCommaSeparatedString(const char *);
  bool
  ReadFromContinuousString(const char *);
  bool
  ReadFromPipeSeparatedString(const char *);
  std::string
  PrintAsContinuousString() const;
  std::string
  PrintAsContinuousUpperCaseString() const;
  std::string
  PrintAsPipeSeparatedString() const;
  std::string
  PrintAsCommaSeparatedString() const;

private:
  union
  {
    uint32_t tag;
    uint16_t tags[2];
    char     bytes[4];
  } ElementTag{0U};
};

inline std::istream &
operator>>(std::istream & _is, Tag & _val)
{
  char c;
  _is >> c;
  uint16_t a, b;
  _is >> std::hex >> a;
  _is >> c;
  _is >> std::hex >> b;
  _is >> c;
  _val.SetGroup(a);
  _val.SetElement(b);
  return _is;
}

inline std::ostream &
operator<<(std::ostream & _os, const Tag & _val)
{
  _os.setf(std::ios::right);
  _os << std::hex << '(' << std::setw(4) << std::setfill('0') << _val[0] << ',' << std::setw(4) << std::setfill('0')
      << _val[1] << ')' << std::setfill(' ') << std::dec;
  return _os;
}

} // end namespace mdcm

#endif // MDCMTAG_H
