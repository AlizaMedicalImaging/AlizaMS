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

#include "mdcmByteValue.h"
#include <cstring>
#include <iterator>
#include <iomanip>
#include <algorithm>

namespace mdcm
{

ByteValue::ByteValue(const char * array, const VL & vl)
  : Internal(array, array + vl)
  , Length(vl)
{
  if (vl.IsOdd())
  {
    mdcmDebugMacro("Odd length");
    Internal.resize(vl + 1);
    ++Length;
  }
}

ByteValue::ByteValue(std::vector<char> & v)
  : Internal(v)
  , Length((uint32_t)v.size())
{}

ByteValue::~ByteValue()
{
  Internal.clear();
}

void
ByteValue::PrintASCII(std::ostream & os, VL maxlength) const
{
  VL length = std::min(maxlength, Length);
  // Special case for VR::UI, do not print the trailing \0
  if (length && length == Length)
  {
    if (Internal[length - 1] == 0)
    {
      length = length - 1;
    }
  }
  std::vector<char>::const_iterator it = Internal.cbegin();
  for (; it != Internal.cbegin() + length; ++it)
  {
    const char & c = *it;
    if (!(isprint((unsigned char)c) || isspace((unsigned char)c)))
      os << ".";
    else
      os << c;
  }
}

void
ByteValue::PrintHex(std::ostream & os, VL maxlength) const
{
  std::ios oldState(NULL);
  oldState.copyfmt(os);
  VL                                length = std::min(maxlength, Length);
  std::vector<char>::const_iterator it = Internal.begin();
  os << std::hex;
  for (; it != Internal.begin() + length; ++it)
  {
    uint8_t v = *it;
    if (it != Internal.begin())
      os << "\\";
    os << std::setw(2) << std::setfill('0') << (uint16_t)v;
  }
  os << std::dec;
  os.copyfmt(oldState);
}

void
ByteValue::PrintGroupLength(std::ostream & os)
{
  assert(Length == 2);
  (void)os;
}

bool
ByteValue::IsEmpty() const
{
  return Length == 0;
}

VL
ByteValue::GetLength() const
{
  return Length;
}

VL
ByteValue::ComputeLength() const
{
  return Length + Length % 2;
}

void
ByteValue::SetLength(VL vl)
{
  VL l(vl);
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
  // CompressedLossy.dcm
  if (l.IsUndefined())
  {
    throw std::logic_error("Can not SetLength, undefined");
  }
  if (l.IsOdd())
  {
    mdcmAlwaysWarnMacro("Odd length value field, trying to workaround");
    ++l;
  }
#else
  assert(!l.IsUndefined() && !l.IsOdd());
#endif
  // Can not use reserve for now, need to implement:
  // STL - vector<> and istream
  // http://groups.google.com/group/comp.lang.c++/msg/37ec052ed8283e74
  try
  {
    Internal.resize(l);
  }
  catch (...)
  {
    throw std::logic_error("Can not resize Internal, exception");
  }
  Length = vl;
}

void
ByteValue::Append(ByteValue const & bv)
{
  Internal.insert(Internal.end(), bv.Internal.begin(), bv.Internal.end());
  Length += bv.Length;
  assert(Internal.size() % 2 == 0 && Internal.size() == Length);
}

void
ByteValue::Clear()
{
  Internal.clear();
}

const char *
ByteValue::GetPointer() const
{
  if (!Internal.empty())
    return &Internal[0];
  return NULL;
}

const void *
ByteValue::GetVoidPointer() const
{
  if (!Internal.empty())
    return &Internal[0];
  return NULL;
}

void *
ByteValue::GetVoidPointer()
{
  if (!Internal.empty())
    return &Internal[0];
  return NULL;
}

void
ByteValue::Fill(char c)
{
  std::vector<char>::iterator it = Internal.begin();
  for (; it != Internal.end(); ++it)
    *it = c;
}

bool
ByteValue::GetBuffer(char * buffer, unsigned long long length) const
{
  if (length <= Internal.size())
  {
    if (!Internal.empty())
      memcpy(buffer, &Internal[0], length);
    return true;
  }
  mdcmAlwaysWarnMacro("Could not handle length = " << length);
  return false;
}

bool
ByteValue::WriteBuffer(std::ostream & os) const
{
  if (Length)
  {
    assert(!(Internal.size() % 2));
    os.write(&Internal[0], Internal.size());
  }
  return true;
}

void
ByteValue::SetLengthOnly(VL vl)
{
  Length = vl;
}

} // end namespace mdcm
