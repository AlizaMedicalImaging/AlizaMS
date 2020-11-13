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
#include "mdcmException.h"
#include <algorithm>
#include <cstring>

namespace mdcm
{

void ByteValue::SetLength(VL vl)
{
  VL l(vl);
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
  // CompressedLossy.dcm
  if(l.IsUndefined())
  {
#ifndef MDCM_DONT_THROW
    throw Exception("Impossible");
#endif
  }
  if(l.IsOdd())
  {
    mdcmDebugMacro("BUGGY HEADER: Your dicom contain odd length value field.");
    ++l;
  }
#else
  assert(!l.IsUndefined() && !l.IsOdd());
#endif
  // I cannot use reserve for now. I need to implement:
  // STL - vector<> and istream
  // http://groups.google.com/group/comp.lang.c++/msg/37ec052ed8283e74

  //#define SHORT_READ_HACK
  try
  {
#ifdef SHORT_READ_HACK
    if(l <= 0xff)
#endif
    Internal.resize(l);
  }
  catch(...)
  {
#ifndef MDCM_DONT_THROW
    throw Exception("Impossible to allocate");
#endif
  }
  Length = vl;
}

void ByteValue::PrintASCII(std::ostream &os, VL maxlength) const
{
  VL length = std::min(maxlength, Length);
  // Special case for VR::UI, do not print the trailing \0
  if(length && length == Length)
  {
    if(Internal[length-1] == 0)
    {
      length = length - 1;
    }
  }
  // I cannot check IsPrintable some file contains \2 or \0 in a VR::LO element
  // See: acr_image_with_non_printable_in_0051_1010.acr
  std::vector<char>::const_iterator it = Internal.begin();
  for(; it != Internal.begin()+length; ++it)
  {
    const char &c = *it;
    if (!(isprint((unsigned char)c) || isspace((unsigned char)c))) os << ".";
    else os << c;
  }
}

void ByteValue::PrintHex(std::ostream &os, VL maxlength) const
{
  std::ios oldState(NULL);
  oldState.copyfmt(os);
  VL length = std::min(maxlength, Length);
  std::vector<char>::const_iterator it = Internal.begin();
  os << std::hex;
  for(; it != Internal.begin()+length; ++it)
  {
    uint8_t v = *it;
    if(it != Internal.begin()) os << "\\";
    os << std::setw(2) << std::setfill('0') << (uint16_t)v;
  }
  os << std::dec;
  os.copyfmt(oldState);
}

bool ByteValue::GetBuffer(char * buffer, unsigned long long length) const
{
  if(length <= Internal.size())
  {
    if (!Internal.empty()) memcpy(buffer, &Internal[0], length);
    return true;
  }
  mdcmDebugMacro("Could not handle length= " << length);
  return false;
}

void ByteValue::PrintPNXML(std::ostream &os) const
{
  int count1 , count2;
  count1=count2=1;
  os << "<PersonName number = \"" << count1 << "\" >\n" ;
  os << "<SingleByte>\n<FamilyName> " ;
  std::vector<char>::const_iterator it = Internal.begin();
  for(; it != (Internal.begin() + Length); ++it)
  {
    const char &c = *it;
    if (c == '^')
    {
      if(count2==1)
      {
        os << "</FamilyName>\n";
        os << "<GivenName> ";
        count2++;
      }
      if(count2==2)
      {
        os << "</GivenName>\n";
        os << "<MiddleName> ";
        count2++;
      }
      else if(count2==3)
      {
        os << "</MiddleName>\n";
        os << "<NamePrefix> ";
        count2++;
      }
      else if(count2==4)
      {
        os << "</NamePrefix>\n";
        os << "<NameSuffix> ";
        count2++;
      }
      else
      {
        //in the rare case there are more ^ characters
        assert("Name components exceeded");
      }
    }
    else if (c == '=')
    {
      if(count2==1)
      {
        os << "</FamilyName>\n";
      }
      if(count2==2)
      {
        os << "</GivenName>\n";
      }
      else if(count2==3)
      {
        os << "</MiddleName>\n";
      }
      else if(count2==4)
      {
        os << "</NamePrefix>\n";
      }
      else if(count2==5)
      {
        os << "</NameSuffix>\n";
      }
      if(count1==1)
      {
        os << "</SingleByte>\n";
        os << "<Ideographic> \n<FamilyName> ";
        count1++;
      }
      else if(count1==2)
      {
        os << "</Ideographic>\n";
        os << "<Phonetic> \n<FamilyName> ";
        count1++;
      }
      else if(count1==3)
      {
        os << "</Phonetic> \n<FamilyName> \n";
        count1++;
      }
      else
      {
        assert("Impossible - only 3 names allowed");
      }
      count2=1;
    }
    else if (!(isprint((unsigned char)c)))
      os << ".";
    else if(c == '&')
      os << "&amp;";
    else if(c == '<')
      os << "&lt;";
    else if(c == '>')
      os << "&gt;";
    else if(c == '\'')
      os << "&apos;";
    else if(c == '\"')
      os << "&quot;";
    else
      os << c;
  }
  if(count2==1)
  {
    os << "</FamilyName>\n";
  }
  if(count2==2)
  {
    os << "</GivenName>\n";
  }
  else if(count2==3)
  {
    os << "</MiddleName>\n";
  }
  else if(count2==4)
  {
    os << "</NamePrefix>\n";
  }
  else if(count2==5)
  {
    os << "</NameSuffix>\n";
  }
  if(count1==1)
  {
    os << "</SingleByte>\n";
  }
  else if(count1==2)
  {
    os << "</Ideographic>\n";
  }
  else if(count1==3)
  {
    os << "</Phonetic>\n";
  }
  os << "</PersonName>";
}

void ByteValue::PrintASCIIXML(std::ostream &os) const
{
  // Special case for VR::UI, do not print the trailing \0
  int count = 1;
  os << "<Value number = \"" << count << "\" >";
  std::vector<char>::const_iterator it = Internal.begin();
  for(; it != (Internal.begin() + Length); ++it)
  {
    const char &c = *it;
    if (c == '\\')
    {
      count++;
      os << "</Value>\n";
      os << "<Value number = \"" << count << "\" >";
    }
    else if (!c)
    {
      ;;// \0 is found
    }
    else if(c == '&')
      os << "&amp;";
    else if(c == '<')
      os << "&lt;";
    else if(c == '>')
      os << "&gt;";
    else if(c == '\'')
      os << "&apos;";
    else if(c == '\"')
      os << "&quot;";
    else
      os << c;
  }
  os << "</Value>\n";
}

void ByteValue::PrintHexXML(std::ostream & os) const
{
  std::ios oldState(NULL);
  oldState.copyfmt(os);
  os << std::hex;
  std::vector<char>::const_iterator it = Internal.begin();
  for(; it != Internal.begin() + Length; ++it)
  {
    uint8_t v = *it;
    if(it != Internal.begin()) os << "\\";
    os << std::setw(2) << std::setfill('0') << (uint16_t)v;
  }
  os << std::dec;
  os.copyfmt(oldState);
}
 
void ByteValue::Append(ByteValue const & bv)
{
  Internal.insert(Internal.end(), bv.Internal.begin(), bv.Internal.end());
  Length += bv.Length;
  assert(Internal.size() % 2 == 0 && Internal.size() == Length);
}
   
} // end namespace mdcm
