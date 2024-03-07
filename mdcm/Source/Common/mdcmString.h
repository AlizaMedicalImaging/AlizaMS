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
#ifndef MDCMSTRING_H
#define MDCMSTRING_H

#include "mdcmTypes.h"

namespace mdcm
{

// Do not export
template <char TDelimiter = '\\', unsigned int TMaxLength = 64, char TPadChar = ' '>
class String : public std::string
{
  // VR UI used \0 for pad character, while ASCII space char
  static_assert(TPadChar == ' ' || TPadChar == 0, "");

public:
  using std::string::value_type;
  using std::string::size_type;

  String() = default;

  String(const value_type * s)
    : std::string(s)
  {
    if (this->size() % 2 != 0)
    {
      push_back(TPadChar);
    }
  }

  String(const value_type * s, size_type n)
    : std::string(s, n)
  {
    // passed a const char* pointer, so s[n] == 0 (garanteed)
    if (n % 2 != 0)
    {
      push_back(TPadChar);
    }
  }

  String(const std::string & s, size_type pos = 0, size_type n = std::string::npos)
    : std::string(s, pos, n)
  {
    // Might already have been padded with a trailing \0
    if (this->size() % 2 != 0)
    {
      push_back(TPadChar);
    }
  }

  // Trailing \0 might be lost in this operation
  operator const char *() const { return this->c_str(); }

  bool
  IsValid() const
  {
    const size_type l = this->size();
    if (l > TMaxLength)
      return false;
    return true;
  }

  mdcm::String<TDelimiter, TMaxLength, TPadChar>
  Truncate() const
  {
    if (IsValid())
      return *this;
    std::string str = *this;
    str.resize(TMaxLength);
    return str;
  }

  std::string
  Trim() const
  {
    std::string str = *this;
    size_type   pos1 = str.find_first_not_of(' ');
    size_type   pos2 = str.find_last_not_of(' ');
    str = str.substr((pos1 == std::string::npos) ? 0 : pos1,
                     (pos2 == std::string::npos) ? (str.size() - 1) : (pos2 - pos1 + 1));
    return str;
  }

  static std::string
  Trim(const char * input)
  {
    if (!input)
      return std::string("");
    std::string str = input;
    size_type   pos1 = str.find_first_not_of(' ');
    size_type   pos2 = str.find_last_not_of(' ');
    str = str.substr((pos1 == std::string::npos) ? 0 : pos1,
                     (pos2 == std::string::npos) ? (str.size() - 1) : (pos2 - pos1 + 1));
    return str;
  }
};

template <char TDelimiter, unsigned int TMaxLength, char TPadChar>
inline std::istream &
operator>>(std::istream & is, String<TDelimiter, TMaxLength, TPadChar> & ms)
{
  if (is)
  {
    std::getline(is, ms, TDelimiter);
    // No such thing as std::get where the delim char would be left, so manually add it back,
    // hopefully this is the right thing to do (no overhead).
    if (!is.eof())
      is.putback(TDelimiter);
  }
  return is;
}

} // end namespace mdcm

#endif // MDCMSTRING_H
