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
friend std::ostream &operator<<(std::ostream & os, const Fragment & val);
public:
  Fragment() : DataElement(Tag(0xfffe, 0xe000), 0) {}
  VL GetLength() const;
  VL ComputeLength() const;

  template <typename TSwap> bool Read(std::istream & is)
  {
    const bool ok = ReadPreValue<TSwap>(is);
    (void)ok;
    return ReadValue<TSwap>(is);
  }

  template <typename TSwap> bool ReadPreValue(std::istream & is)
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    TagField.Read<TSwap>(is);
    if(!is)
    {
      mdcmAlwaysWarnMacro("Problem #1");
      return false;
    }
    if(!ValueLengthField.Read<TSwap>(is))
    {
      mdcmAlwaysWarnMacro("Problem #2");
      return false;
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    if(TagField != itemStart && TagField != seqDelItem)
    {
      mdcmAlwaysWarnMacro("Problem #3");
      return false;
    }
#endif
    return true;
  }

  template <typename TSwap> bool ReadValue(std::istream & is)
  {
    const Tag itemStart(0xfffe,0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if(!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    ValueField = bv;
    if(!is) return false;
    return true;
  }

  template <typename TSwap> bool ReadBacktrack(std::istream & is)
  {
    if(!is) return false;
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    bool cont = true;
    const std::streampos start = is.tellg();
    const int max = 10;
    int offset = 0;
    while(cont)
    {
      TagField.Read<TSwap>(is);
      if(TagField != itemStart && TagField != seqDelItem)
      {
        ++offset;
        is.seekg((std::streampos)((size_t)start - offset));
        mdcmWarningMacro("Fuzzy Search, backtrack: " << (start - is.tellg()) << " Offset: " << is.tellg());
        if(offset > max)
        {
          mdcmAlwaysWarnMacro("Impossible to backtrack");
          return false;
        }
      }
      else
      {
        cont = false;
      }
    }
    assert(TagField == itemStart || TagField == seqDelItem);
    if(!ValueLengthField.Read<TSwap>(is))
    {
      return false;
    }
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if(!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    ValueField = bv;
    return true;
  }

  template <typename TSwap> std::ostream & Write(std::ostream & os) const
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    if(!TagField.Write<TSwap>(os))
    {
      assert(0 && "Should not happen");
      return os;
    }
    assert(TagField == itemStart || TagField == seqDelItem);
    const ByteValue * bv = GetByteValue();
    // VL
    // The following piece of code is hard to read in order to support such broken file as
    // CompressedLossy.dcm
    if(IsEmpty())
    {
      VL zero = 0;
      if(!zero.Write<TSwap>(os))
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
      if(!actuallen.Write<TSwap>(os))
      {
        assert(0 && "Should not happen");
        return os;
      }
    }
    if(ValueLengthField && bv)
    {
      assert(bv);
      assert(bv->GetLength() == ValueLengthField);
      if(!bv->Write<TSwap>(os))
      {
        assert(0 && "Should not happen");
        return os;
      }
    }
    return os;
  }
};

inline std::ostream &operator<<(std::ostream & os, const Fragment & val)
{
  os << "Tag: " << val.TagField;
  os << "\tVL: " << val.ValueLengthField;
  if(val.ValueField)
  {
    os << "\t" << *(val.ValueField);
  }
  return os;
}

} // end namespace mdcm

#endif //MDCMFRAGMENT_H
