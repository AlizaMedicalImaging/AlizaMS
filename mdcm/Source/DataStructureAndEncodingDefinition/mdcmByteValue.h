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
#ifndef MDCMBYTEVALUE_H
#define MDCMBYTEVALUE_H

#include "mdcmValue.h"
#include "mdcmTrace.h"
#include "mdcmVL.h"
#include <vector>
#include <iostream>

namespace mdcm
{

/**
 * Class to represent binary value (array of bytes)
 */

class MDCM_EXPORT ByteValue : public Value
{
public:
  ByteValue() = default;
  explicit ByteValue(const char *, const VL &);
  explicit ByteValue(const std::vector<char> &);
  ByteValue(ByteValue && val) noexcept;
  ~ByteValue() = default;

  operator const std::vector<char> &() const { return Internal; }

  ByteValue &
  operator=(const ByteValue & other)
  {
    Internal = other.Internal;
    Length = other.Length;
    return *this;
  }

  ByteValue &
  operator=(ByteValue && other) noexcept
  {
    if (this != &other)
    {
      Internal = std::move(other.Internal);
      Length = other.Length;
      other.Length = 0;
    }
    return *this;
  }

  bool
  operator==(const ByteValue & val) const
  {
    if (Length != val.Length)
      return false;
    if (Internal == val.Internal)
      return true;
    return false;
  }

  bool
  operator==(const Value & val) const override
  {
    const ByteValue & bv = dynamic_cast<const ByteValue &>(val);
    return Length == bv.Length && Internal == bv.Internal;
  }

  template <typename TSwap, typename TType>
  std::istream &
  Read(std::istream & is, bool readvalues = true)
  {
    // If Length is odd we have detected that in SetLength
    // and calling std::vector::resize make sure to allocate *AND*
    // initialize values to 0 so we are sure to have a \0 at the end
    // even in this case
    if (Length)
    {
      if (readvalues)
      {
        is.read(Internal.data(), Length);
        assert(Internal.size() == Length || Internal.size() == Length + 1);
        TSwap::SwapArray(static_cast<TType *>(GetVoidPointer()), Internal.size() / sizeof(TType));
      }
      else
      {
        is.seekg(Length, std::ios::cur);
      }
    }
    return is;
  }

  template <typename TSwap>
  std::istream &
  Read(std::istream & is)
  {
    return Read<TSwap, uint8_t>(is);
  }

  template <typename TSwap, typename TType>
  const std::ostream &
  Write(std::ostream & os) const
  {
    assert(!(Internal.size() % 2));
    if (!Internal.empty())
    {
      std::vector<char> copy = Internal;
      TSwap::SwapArray(static_cast<TType *>(static_cast<void *>(copy.data())), Internal.size() / sizeof(TType));
      os.write(copy.data(), copy.size());
    }
    return os;
  }

  template <typename TSwap>
  const std::ostream &
  Write(std::ostream & os) const
  {
    return Write<TSwap, uint8_t>(os);
  }

  void
  PrintASCII(std::ostream &, VL) const;
  void
  PrintHex(std::ostream &, VL) const;
  void
  PrintGroupLength(std::ostream &);
  bool
  IsEmpty() const;
  VL
  GetLength() const override;
  VL
  ComputeLength() const;
  void SetLength(VL) override;
  void
  Append(const ByteValue &);
  void
  Clear() override;
  const char *
  GetPointer() const;
  const void *
  GetVoidPointer() const;
  void *
  GetVoidPointer();
  void
  Fill(char);
  bool
  GetBuffer(char *, unsigned long long) const;
  bool
  WriteBuffer(std::ostream &) const;

protected:
  void SetLengthOnly(VL) override;

private:
  std::vector<char> Internal{};
  // WARNING Length is not Internal.size()
  VL Length{};
};

} // end namespace mdcm

#endif // MDCMBYTEVALUE_H
