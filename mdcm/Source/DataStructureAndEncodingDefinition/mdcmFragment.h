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
#ifndef MDCMFRAGMENT_H
#define MDCMFRAGMENT_H

#include "mdcmDataElement.h"
#include "mdcmByteValue.h"
#include "mdcmSmartPointer.h"
#include "mdcmParseException.h"

namespace mdcm
{

/**
 * Class to represent a Fragment
 */
class MDCM_EXPORT Fragment : public DataElement
{
  friend std::ostream &
  operator<<(std::ostream &, const Fragment &);

public:
  Fragment()
    : DataElement(Tag(0xfffe, 0xe000), 0)
  {}

  VL
  GetLength() const;

  VL
  ComputeLength() const;

  template <typename TSwap>
  std::istream &
  Read(std::istream & is)
  {
    ReadPreValue<TSwap>(is);
    return ReadValue<TSwap>(is);
  }

  template <typename TSwap>
  std::istream &
  ReadPreValue(std::istream & is)
  {
    TagField.Read<TSwap>(is);
    if (!is)
    {
      throw std::logic_error("Problem #1");
    }
    if (!ValueLengthField.Read<TSwap>(is))
    {
      throw std::logic_error("Problem #2");
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe, 0xe0dd);
    if (TagField != itemStart && TagField != seqDelItem)
    {
      throw std::logic_error("Problem #3");
    }
#endif
    return is;
  }

  template <typename TSwap>
  std::istream &
  ReadValue(std::istream & is)
  {
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if (!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    ValueField = bv;
    return is;
  }

  template <typename TSwap>
  std::istream &
  ReadBacktrack(std::istream & is)
  {
    const Tag            itemStart(0xfffe, 0xe000);
    const Tag            seqDelItem(0xfffe, 0xe0dd);
    bool                 cont = true;
    const std::streampos start = is.tellg();
    const int            max = 10;
    int                  offset = 0;
    while (cont)
    {
      TagField.Read<TSwap>(is);
      assert(is);
      if (TagField != itemStart && TagField != seqDelItem)
      {
        ++offset;
        is.seekg(static_cast<std::streampos>(static_cast<size_t>(start) - offset));
        mdcmWarningMacro("Fuzzy search, backtrack: " << (start - is.tellg()) << " Offset: " << is.tellg());
        if (offset > max)
        {
          throw std::logic_error("Impossible to backtrack");
        }
      }
      else
      {
        cont = false;
      }
    }
    assert(TagField == itemStart || TagField == seqDelItem);
    if (!ValueLengthField.Read<TSwap>(is))
    {
      return is;
    }
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if (!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    ValueField = bv;
    return is;
  }

  template <typename TSwap>
  std::ostream &
  Write(std::ostream & os) const
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe, 0xe0dd);
    if (!TagField.Write<TSwap>(os))
    {
      assert(0 && "Should not happen");
      return os;
    }
    assert(TagField == itemStart || TagField == seqDelItem);
    const ByteValue * bv = GetByteValue();
    // broken file e.g. CompressedLossy.dcm
    if (IsEmpty())
    {
      VL zero = 0;
      if (!zero.Write<TSwap>(os))
      {
        assert(0 && "Should not happen");
        return os;
      }
    }
    else
    {
      assert(ValueLengthField);
      assert(!ValueLengthField.IsUndefined());
      const VL actuallen = bv->ComputeLength();
      assert(actuallen == ValueLengthField || actuallen == ValueLengthField + 1);
      if (!actuallen.Write<TSwap>(os))
      {
        assert(0 && "Should not happen");
        return os;
      }
    }
    if (ValueLengthField && bv)
    {
      assert(bv);
      assert(bv->GetLength() == ValueLengthField);
      if (!bv->Write<TSwap>(os))
      {
        assert(0 && "Should not happen");
        return os;
      }
    }
    return os;
  }
};

inline std::ostream &
operator<<(std::ostream & os, const Fragment & val)
{
  os << "Tag: " << val.TagField;
  os << "\tVL: " << val.ValueLengthField;
  if (val.ValueField)
  {
    os << "\t" << *(val.ValueField);
  }
  return os;
}

} // end namespace mdcm

#endif // MDCMFRAGMENT_H
