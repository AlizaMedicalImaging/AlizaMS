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

namespace mdcm_ns
{

/**
 * \brief Class to represent a Fragment
 */
class MDCM_EXPORT Fragment : public DataElement
{
public:
  Fragment() : DataElement(Tag(0xfffe, 0xe000), 0) {}
  friend std::ostream &operator<<(std::ostream & os, const Fragment & val);

  VL GetLength() const;

  VL ComputeLength() const;

  template <typename TSwap>
  std::istream & Read(std::istream &is)
  {
    ReadPreValue<TSwap>(is);
    return ReadValue<TSwap>(is);
  }

  template <typename TSwap>
  std::istream & ReadPreValue(std::istream & is)
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);

    TagField.Read<TSwap>(is);
    if(!is)
    {
#ifndef MDCM_DONT_THROW
      throw Exception("Problem #1");
#endif
      return is;
    }
    if(!ValueLengthField.Read<TSwap>(is))
    {
#ifndef MDCM_DONT_THROW
      throw Exception("Problem #2");
#endif
      return is;
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    if(TagField != itemStart && TagField != seqDelItem)
    {
#ifndef MDCM_DONT_THROW
      throw Exception("Problem #3");
#endif
    }
#endif
    return is;
  }

  template <typename TSwap>
  std::istream &ReadValue(std::istream &is)
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if(!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
#ifndef MDCM_DONT_THROW
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
#endif
      return is;
    }
    ValueField = bv;
    return is;
  }

  template <typename TSwap>
  std::istream &ReadBacktrack(std::istream &is)
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);

    bool cont = true;
    const std::streampos start = is.tellg();
    const int max = 10;
    int offset = 0;
    while(cont)
    {
      TagField.Read<TSwap>(is);
      assert(is);
      if(TagField != itemStart && TagField != seqDelItem)
      {
        ++offset;
        is.seekg((std::streampos)((size_t)start - offset));
        mdcmWarningMacro("Fuzzy Search, backtrack: " << (start - is.tellg()) << " Offset: " << is.tellg());
        if(offset > max)
        {
          mdcmErrorMacro("Giving up");
#ifndef MDCM_DONT_THROW
          throw "Impossible to backtrack";
#endif
          return is;
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
      return is;
    }

    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if(!bv->Read<TSwap>(is))
    {
      // Fragment is incomplete, but is a itemStart, let's try to push it anyway
      mdcmWarningMacro("Fragment could not be read");
      ValueField = bv;
#ifndef MDCM_DONT_THROW
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
#endif
      return is;
    }
    ValueField = bv;
    return is;
  }

  template <typename TSwap>
  std::ostream & Write(std::ostream &os) const
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

} // end namespace mdcm_ns

#endif //MDCMFRAGMENT_H
