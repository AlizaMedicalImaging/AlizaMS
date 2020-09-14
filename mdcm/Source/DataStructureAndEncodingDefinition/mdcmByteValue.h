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
#include <iterator>
#include <iomanip>
#include <algorithm>

namespace mdcm
{

/**
 * Class to represent binary value (array of bytes)
 */

class MDCM_EXPORT ByteValue : public Value
{
public:
  ByteValue(const char * array = NULL, VL const & vl = 0)
    :
    Internal(array, array+vl),
    Length(vl)
  {
      if(vl.IsOdd())
      {
        mdcmDebugMacro("Odd length");
        Internal.resize(vl+1);
        Length++;
      }
  }

  ByteValue(std::vector<char> & v)
    :
    Internal(v),Length((uint32_t)v.size()) {}

  ~ByteValue() { Internal.clear(); }
  void PrintASCII(std::ostream &, VL) const;
  void PrintHex(std::ostream &, VL) const;

  void PrintGroupLength(std::ostream & os)
  {
    assert(Length == 2);
    (void)os;
  }

  bool IsEmpty() const { return Length == 0; }
  VL GetLength() const { return Length; }

  VL ComputeLength() const { return Length + Length % 2; }
  void SetLength(VL vl);

  operator const std::vector<char>& () const { return Internal; }

  ByteValue &operator=(const ByteValue & val)
  {
    Internal = val.Internal;
    Length = val.Length;
    return *this;
  }

  bool operator==(const ByteValue & val) const
  {
    if(Length != val.Length)     return false;
    if(Internal == val.Internal) return true;
    return false;
  }

  bool operator==(const Value &val) const
  {
    const ByteValue &bv = dynamic_cast<const ByteValue&>(val);
    return Length == bv.Length && Internal == bv.Internal;
  }

  void Append(ByteValue const & bv);

  void Clear() {  Internal.clear(); }

  const char * GetPointer() const
  {
    if(!Internal.empty()) return &Internal[0];
    return NULL;
  }

  const void * GetVoidPointer() const
  {
    if(!Internal.empty()) return &Internal[0];
    return NULL;
  }

  void * GetVoidPointer()
  {
    if(!Internal.empty()) return &Internal[0];
    return NULL;
  }

  void Fill(char c)
  {
    std::vector<char>::iterator it = Internal.begin();
    for(; it != Internal.end(); ++it) *it = c;
  }

  bool GetBuffer(char*, unsigned long long) const;

  bool WriteBuffer(std::ostream & os) const
  {
    if(Length)
    {
      assert(!(Internal.size() % 2));
      os.write(&Internal[0], Internal.size());
    }
    return true;
  }

  template <typename TSwap, typename TType>
  std::istream & Read(std::istream & is, bool readvalues = true)
  {
    // If Length is odd we have detected that in SetLength
    // and calling std::vector::resize make sure to allocate *AND*
    // initialize values to 0 so we are sure to have a \0 at the end
    // even in this case
    if(Length)
    {
      if(readvalues)
      {
        is.read(&Internal[0], Length);
        assert(Internal.size() == Length || Internal.size() == Length + 1);
        TSwap::SwapArray((TType*)GetVoidPointer(), Internal.size()/sizeof(TType));
      }
      else
      {
        is.seekg(Length, std::ios::cur);
      }
    }
    return is;
  }

  template <typename TSwap>
  std::istream & Read(std::istream & is)
  {
    return Read<TSwap,uint8_t>(is);
  }

  template <typename TSwap, typename TType>
  std::ostream const & Write(std::ostream & os) const
  {
    assert(!(Internal.size() % 2));
    if(!Internal.empty())
    {
      std::vector<char> copy = Internal;
      TSwap::SwapArray(
        (TType*)(void*)&copy[0], Internal.size()/sizeof(TType));
      os.write(&copy[0], copy.size());
    }
    return os;
  }

  template <typename TSwap>
  std::ostream const & Write(std::ostream & os) const
  {
    return Write<TSwap,uint8_t>(os);
  }

  /**
   * \brief  Checks whether a 'ByteValue' is printable or not (in order
   *         to avoid corrupting the terminal of invocation when printing)
   *         I don't think this function is working since it does not handle
   *         UNICODE or character set...
   */
  bool IsPrintable(VL length) const
  {
    assert(length <= Length);
    for(unsigned int i=0; i<length; i++)
    {
      if (i == (length-1) && Internal[i] == '\0') continue;
      if (!(isprint((unsigned char)Internal[i]) ||
              isspace((unsigned char)Internal[i])))
      {
        return false;
      }
    }
    return true;
  }

  void PrintPNXML(std::ostream & os) const;
  void PrintASCIIXML(std::ostream & os) const;
  void PrintHexXML(std::ostream & os) const;

protected:
  void Print(std::ostream & os) const
  {
    if(!Internal.empty())
    {
      if(IsPrintable(Length))
      {
        std::vector<char>::size_type length = Length;
        if(Internal.back() == 0) --length;
        std::copy(
          Internal.begin(),
          Internal.begin()+length,
          std::ostream_iterator<char>(os));
      }
      else
      {
        os << "Loaded:" << Internal.size();
      }
    }
    else
    {
      os << "(no value available)";
    }
  }

  void SetLengthOnly(VL vl) { Length = vl; }

private:
  std::vector<char> Internal;

  // WARNING Length IS NOT Internal.size() some *featured* DICOM
  // implementation define odd length, we always load them as even number
  // of byte, so we need to keep the right Length
  VL Length;
};

} // end namespace mdcm

#endif //MDCMBYTEVALUE_H
