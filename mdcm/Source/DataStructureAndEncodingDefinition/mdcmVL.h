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
#ifndef MDCMVL_H
#define MDCMVL_H

#include "mdcmTypes.h"
#include "mdcmTrace.h"
#include <iostream>

namespace mdcm
{

/**
 * Value Length
 * This is a 4bytes value! Do not try to use it for 2bytes value length
 */
class MDCM_EXPORT VL
{
public:
  typedef uint32_t Type;

  VL(uint32_t vl = 0)
    : ValueLength(vl)
  {}
  static uint32_t
  GetVL32Max()
  {
    return 0xFFFFFFFF;
  }
  static uint16_t
  GetVL16Max()
  {
    return 0xFFFF;
  }

  bool
  IsUndefined() const
  {
    return ValueLength == 0xFFFFFFFF;
  }

  void
  SetToUndefined()
  {
    ValueLength = 0xFFFFFFFF;
  }

  bool
  IsOdd() const
  {
    return (!IsUndefined() && ValueLength % 2);
  }

  VL &
  operator+=(VL const & vl)
  {
    ValueLength += vl.ValueLength;
    return *this;
  }

  VL &
  operator++()
  {
    ++ValueLength;
    return *this;
  }

  VL
  operator++(int)
  {
    uint32_t tmp(ValueLength);
    ++ValueLength;
    return tmp;
  }

  operator uint32_t() const { return ValueLength; }

  VL
  GetLength() const
  {
    // TODO check cannot call this function from an Explicit element
    return 4;
  }

  friend std::ostream &
  operator<<(std::ostream &, const VL &);

  template <typename TSwap>
  std::istream &
  Read(std::istream & is)
  {
    is.read((char *)(&ValueLength), sizeof(uint32_t));
    TSwap::SwapArray(&ValueLength, 1);
    return is;
  }

  template <typename TSwap>
  std::istream &
  Read16(std::istream & is)
  {
    uint16_t copy;
    is.read((char *)(&copy), sizeof(uint16_t));
    TSwap::SwapArray(&copy, 1);
    ValueLength = copy;
    assert(ValueLength <= 65535 /*UINT16_MAX*/); // ?? doh !
    return is;
  }

  template <typename TSwap>
  const std::ostream &
  Write(std::ostream & os) const
  {
    uint32_t copy = ValueLength;
    if (IsOdd())
    {
      ++copy;
    }
    TSwap::SwapArray(&copy, 1);
    return os.write((char *)(&copy), sizeof(uint32_t));
  }

  template <typename TSwap>
  const std::ostream &
  Write16(std::ostream & os) const
  {
    assert(ValueLength <= 65535);
    uint16_t copy = (uint16_t)ValueLength;
    if (IsOdd())
    {
      ++copy;
    }
    TSwap::SwapArray(&copy, 1);
    return os.write((char *)(&copy), sizeof(uint16_t));
  }

private:
  uint32_t ValueLength;
};

inline std::ostream &
operator<<(std::ostream & os, const VL & val)
{
  os << val.ValueLength;
  return os;
}

} // end namespace mdcm

#endif // MDCMVL_H
