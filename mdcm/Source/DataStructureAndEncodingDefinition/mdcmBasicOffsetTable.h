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

#ifndef MDCMBASICOFFSETTABLE_H
#define MDCMBASICOFFSETTABLE_H

#include "mdcmFragment.h"

namespace mdcm_ns
{
/**
 * \brief Class to represent a BasicOffsetTable
 */

class MDCM_EXPORT BasicOffsetTable : public Fragment
{
public:
  BasicOffsetTable() : Fragment() {}
  friend std::ostream &operator<<(std::ostream & os, const BasicOffsetTable & val);
  template <typename TSwap>
  std::istream & Read(std::istream &is) {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    if(!TagField.Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    if(TagField != itemStart)
    {
#ifndef MDCM_DONT_THROW
      throw "SIEMENS Icon thingy";
#endif
    }
    if(!ValueLengthField.Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    SmartPointer<ByteValue> bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if(!bv->Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    ValueField = bv;
    return is;
  }
};

inline std::ostream &operator<<(std::ostream & os, const BasicOffsetTable & val)
{
  os << " BasicOffsetTable Length=" << val.ValueLengthField << std::endl;
  if(val.ValueField)
  {
    const ByteValue * bv = val.GetByteValue();
    assert(bv);
    os << *bv;
  }

  return os;
}

} // end namespace mdcm_ns

#endif //MDCMBASICOFFSETTABLE_H
