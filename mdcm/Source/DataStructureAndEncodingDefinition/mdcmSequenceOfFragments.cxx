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
#include "mdcmSequenceOfFragments.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmByteValue.h"

namespace mdcm
{

void
SequenceOfFragments::Clear()
{
  Table.SetByteValue("", 0);
  Fragments.clear();
}

SequenceOfFragments::SizeType
SequenceOfFragments::GetNumberOfFragments() const
{
  return Fragments.size();
}

void
SequenceOfFragments::AddFragment(Fragment const & item)
{
  Fragments.push_back(item);
}

VL
SequenceOfFragments::ComputeLength() const
{
  VL length = 0;
  // First the table
  length += Table.GetLength();
  // Then all the fragments
  FragmentVector::const_iterator it = Fragments.cbegin();
  for (; it != Fragments.cend(); ++it)
  {
    const VL fraglen = it->ComputeLength();
    assert(fraglen % 2 == 0);
    length += fraglen;
  }
  assert(SequenceLengthField.IsUndefined());
  length += 8; // seq end delimitor (tag + vl)
  return length;
}

unsigned long long
SequenceOfFragments::ComputeByteLength() const
{
  unsigned long long             r = 0;
  FragmentVector::const_iterator it = Fragments.cbegin();
  for (; it != Fragments.cend(); ++it)
  {
    assert(!it->GetVL().IsUndefined());
    r += it->GetVL();
  }
  return r;
}

bool
SequenceOfFragments::GetFragBuffer(unsigned int fragNb, char * buffer, unsigned long long & length) const
{
  FragmentVector::const_iterator it = Fragments.cbegin();
  {
    const Fragment &  frag = *(it + fragNb);
    const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
    const VL          len = frag.GetVL();
    bv.GetBuffer(buffer, len);
    length = len;
  }
  return true;
}

const Fragment
SequenceOfFragments::GetFragment(SizeType num) const
{
  if (num < Fragments.size())
  {
    FragmentVector::const_iterator it = Fragments.cbegin();
    const Fragment                 frag = *(it + num);
    return frag;
  }
  assert(0);
  const Fragment f;
  return f;
}

bool
SequenceOfFragments::GetBuffer(char * buffer, unsigned long long length) const
{
  FragmentVector::const_iterator it = Fragments.cbegin();
  unsigned long long             total = 0;
  for (; it != Fragments.cend(); ++it)
  {
    const Fragment &  frag = *it;
    const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
    const VL          len = frag.GetVL();
    bv.GetBuffer(buffer, len);
    buffer += len;
    total += len;
  }
  if (total != length)
  {
    assert(0);
    return false;
  }
  return true;
}

bool
SequenceOfFragments::WriteBuffer(std::ostream & os) const
{
  FragmentVector::const_iterator it = Fragments.cbegin();
  unsigned long long             total = 0;
  for (; it != Fragments.cend(); ++it)
  {
    const Fragment &  frag = *it;
    const ByteValue * bv = frag.GetByteValue();
    if (bv)
    {
      const VL len = frag.GetVL();
      bv->WriteBuffer(os);
      total += len;
    }
    else
    {
      assert(0);
    }
  }
  return true;
}

bool
SequenceOfFragments::FillFragmentWithJPEG(Fragment & frag, std::istream & is)
{
  std::vector<unsigned char> jfif;
  unsigned char              byte;
  while (is.read(reinterpret_cast<char *>(&byte), 1))
  {
    jfif.push_back(byte);
    if (byte == 0xd9 && jfif[jfif.size() - 2] == 0xff)
      break;
  }
  const uint32_t len = static_cast<uint32_t>(jfif.size());
  frag.SetByteValue(reinterpret_cast<char *>(&jfif[0]), len);
  return true;
}

} // end namespace mdcm
