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

namespace
{
	static const mdcm::Fragment empty{};
}

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
SequenceOfFragments::AddFragment(const Fragment & item)
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
  for (FragmentVector::const_iterator it = Fragments.cbegin(); it != Fragments.cend(); ++it)
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
  unsigned long long r = 0;
  for (FragmentVector::const_iterator it = Fragments.cbegin(); it != Fragments.cend(); ++it)
  {
    assert(!it->GetVL().IsUndefined());
    r += it->GetVL();
  }
  return r;
}

#if 0
bool
SequenceOfFragments::GetFragBuffer(unsigned int fragNb, char * buffer, unsigned long long & length) const
{
  if (fragNb < Fragments.size())
  {
    const Fragment &  frag = Fragments.at(fragNb);
    const ByteValue & bv = dynamic_cast<const ByteValue &>(frag.GetValue());
    const VL          len = frag.GetVL();
    const bool ok = bv.GetBuffer(buffer, len);
    length = len;
    if (!ok)
    {
      buffer = nullptr;
      length = 0;
      mdcmAlwaysWarnMacro("SequenceOfFragments::GetFragBuffer error");
      return false;
    }
    return true;
  }
  buffer = nullptr;
  length = 0;
  return false;
}
#endif

const Fragment &
SequenceOfFragments::GetFragment(SizeType num) const
{
  if (num < Fragments.size())
  {
    return Fragments.at(num);
  }
  assert(0);
  return empty;
}

bool
SequenceOfFragments::GetBuffer(char * buffer, unsigned long long length) const
{
  unsigned long long total = 0;
  for (FragmentVector::const_iterator it = Fragments.cbegin(); it != Fragments.cend(); ++it)
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
  unsigned long long total = 0;
  for (FragmentVector::const_iterator it = Fragments.cbegin(); it != Fragments.cend(); ++it)
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
  (void)total;
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
  frag.SetByteValue(reinterpret_cast<char *>(jfif.data()), len);
  return true;
}

} // end namespace mdcm
