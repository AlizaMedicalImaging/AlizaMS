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
#ifndef MDCMELEMENT_H
#define MDCMELEMENT_H

#include "mdcmTypes.h"
#include "mdcmVR.h"
#include "mdcmTag.h"
#include "mdcmVM.h"
#include "mdcmByteValue.h"
#include "mdcmDataElement.h"
#include "mdcmSwapper.h"
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <cmath>
#include <cstring>

namespace mdcm
{

/**
 * EncodingImplementation
 *
 */

template <long long T>
class EncodingImplementation;

/**
 * Class which is used to produce compile errors for an
 * invalid combination of template parameters.
 *
 * Invalid combinations have specialized declarations with no
 * definition.
 */
template <long long TVR, int TVM>
class ElementDisableCombinations
{};

template <>
class ElementDisableCombinations<VR::OB, VM::VM1_n>
{};

template <>
class ElementDisableCombinations<VR::OW, VM::VM1_n>
{};

template <>
class ElementDisableCombinations<VR::OL, VM::VM1_n>
{};

template <>
class ElementDisableCombinations<VR::OD, VM::VM1_n>
{};

template <>
class ElementDisableCombinations<VR::OF, VM::VM1_n>
{};

template <>
class ElementDisableCombinations<VR::OV, VM::VM1_n>
{};

template <int TVM>
class ElementDisableCombinations<VR::OB, TVM>;

template <int TVM>
class ElementDisableCombinations<VR::OW, TVM>;

template <int TVM>
class ElementDisableCombinations<VR::OL, TVM>;

template <int TVM>
class ElementDisableCombinations<VR::OD, TVM>;

template <int TVM>
class ElementDisableCombinations<VR::OF, TVM>;

template <int TVM>
class ElementDisableCombinations<VR::OV, TVM>;

/**
 * Element class
 */
template <long long TVR, int TVM>
class Element
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, TVM>)
  };

public:
  typename VRToType<TVR>::Type         Internal[VMToLength<TVM>::Length];
  typedef typename VRToType<TVR>::Type Type;

  static VR
  GetVR()
  {
    return static_cast<VR::VRType>(TVR);
  }

  static VM
  GetVM()
  {
    return static_cast<VM::VMType>(TVM);
  }

  unsigned long
  GetLength() const
  {
    return VMToLength<TVM>::Length;
  }

  const typename VRToType<TVR>::Type *
  GetValues() const
  {
    return Internal;
  }

  const typename VRToType<TVR>::Type &
  GetValue(unsigned int idx = 0) const
  {
    assert(idx < VMToLength<TVM>::Length);
    return Internal[idx];
  }

  typename VRToType<TVR>::Type &
  GetValue(unsigned int idx = 0)
  {
    assert(idx < VMToLength<TVM>::Length);
    return Internal[idx];
  }

  typename VRToType<TVR>::Type operator[](unsigned int idx) const { return GetValue(idx); }

  void
  SetValue(typename VRToType<TVR>::Type v, unsigned int idx = 0)
  {
    assert(idx < VMToLength<TVM>::Length);
    Internal[idx] = v;
  }

  void
  SetFromDataElement(const DataElement & de)
  {
    const ByteValue * bv = de.GetByteValue();
    if (!bv)
      return;
#ifdef MDCM_WORDS_BIGENDIAN
    if (de.GetVR() == VR::UN)
#else
    if (de.GetVR() == VR::UN || de.GetVR() == VR::INVALID)
#endif
    {
      Set(de.GetValue());
    }
    else
    {
      SetNoSwap(de.GetValue());
    }
  }

  DataElement
  GetAsDataElement() const
  {
    DataElement        ret;
    std::ostringstream os;
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetLength(), os);
    ret.SetVR(static_cast<VR::VRType>(TVR));
    assert(ret.GetVR() != VR::SQ);
    if (static_cast<VR::VRType>(VRToEncoding<TVR>::Mode) == VR::VRASCII)
    {
      if (GetVR() != VR::UI)
      {
        if (os.str().size() % 2 != 0)
        {
          os << ' ';
        }
      }
    }
    VL::Type osStrSize = static_cast<VL::Type>(os.str().size());
    ret.SetByteValue(os.str().c_str(), osStrSize);
    return ret;
  }

  void
  Read(std::istream & _is)
  {
    return EncodingImplementation<VRToEncoding<TVR>::Mode>::Read(Internal, GetLength(), _is);
  }
  void

  Write(std::ostream & _os) const
  {
    return EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetLength(), _os);
  }

  void
  Set(const Value & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Read(Internal, GetLength(), ss);
  }

protected:
  void
  SetNoSwap(const Value & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadNoSwap(Internal, GetLength(), ss);
  }
};

struct ignore_char
{
  ignore_char(char c)
    : m_char(c)
  {}
  char m_char;
};
ignore_char const backslash('\\');

inline std::istream &
operator>>(std::istream & in, const ignore_char & ic)
{
  if (!in.eof())
    in.clear(in.rdstate() & ~std::ios_base::failbit);
  if (in.get() != ic.m_char)
    in.setstate(std::ios_base::failbit);
  return in;
}

// Implementation to perform formatted read and write
template <>
class EncodingImplementation<VR::VRASCII>
{
public:
  template <typename T> // may be VRToType<TVR>::Type
  static inline void
  ReadComputeLength(T * data, unsigned int & length, std::istream & _is)
  {
    length = 0;
    if (!data)
      return;
    while (_is >> std::ws >> data[length++] >> std::ws >> backslash)
    {
      ;
    }
  }

  template <typename T> // may be VRToType<TVR>::Type
  static inline void
  Read(T * data, unsigned long length, std::istream & _is)
  {
    if (!data || length < 1)
      return;
    _is >> std::ws >> data[0];
    if (length > 1)
    {
      char sep;
      for (unsigned long i = 1; i < length; ++i)
      {
        _is >> std::ws >> sep;
        _is >> std::ws >> data[i];
      }
    }
  }

  template <typename T>
  static inline void
  ReadNoSwap(T * data, unsigned long length, std::istream & _is)
  {
    Read(data, length, _is);
  }

  template <typename T>
  static inline void
  Write(const T * data, unsigned long length, std::ostream & _os)
  {
    if (!data || length < 1)
      return;
    _os << data[0];
    if (length > 1)
    {
      for (unsigned long i = 1; i < length; ++i)
      {
        _os << '\\' << data[i];
      }
    }
  }

  template <typename T> // may be VRToType<TVR>::Type
  static inline void
  ReadOne(T & data, unsigned long, std::istream & _is)
  {
    _is >> std::ws >> data;
  }

  template <typename T>
  static inline void
  ReadNoSwapOne(T & data, unsigned long l, std::istream & _is)
  {
    ReadOne(data, l, _is);
  }

  template <typename T>
  static inline void
  WriteOne(const T & data, unsigned long, std::ostream & _os)
  {
    _os << data;
  }
};

// S. https://gist.github.com/issakomi/0bcd25b4e373e2e40b6f073431835f4a
static void ds16print(char * buf, double f)
{
  char * line = new char[400]; // overallocated
  memset(line, 0, 400);
  int l = snprintf(line, 399, "%.17g", f);
  if (l > 16)
  {
    const int strl = static_cast<int>(strlen(line));
    int prec = strl > 1 ? strl - 1 : 1; // not required
    l = snprintf(line, 399, "%.*g", prec, f);
    while (l > 16)
    {
      --prec;
      l = snprintf(line, 399,"%.*g", prec, f);
    }
  }
  strcpy(buf, line);
  delete [] line;
}

template <>
inline void
EncodingImplementation<VR::VRASCII>::Write(const double * data, unsigned long length, std::ostream & _os)
{
  if (!data || length < 1)
    return;
  for (unsigned long i = 0; i < length; ++i)
  {
    char buf[17];
    memset(buf, 0, 17);
    ds16print(buf, data[i]);
    if (i == 0)
      _os << buf;
    else
      _os << '\\' << buf;
  }
}

template <>
class EncodingImplementation<VR::VRBINARY>
{
public:
  template <typename T> // may be VRToType<TVR>::Type
  static inline void
  ReadComputeLength(T * data, unsigned int & length, std::istream & _is)
  {
    if (!data)
    {
      length = 0;
      return;
    }
    const unsigned int type_size = sizeof(T);
    length /= type_size;
    for (unsigned long i = 0; i < length; ++i)
    {
      _is.read(reinterpret_cast<char *>(data + i), type_size);
    }
  }

  template <typename T>
  static inline void
  ReadNoSwap(T * data, unsigned long length, std::istream & _is)
  {
    if (!data || length < 1)
      return;
    const unsigned int type_size = sizeof(T);
    for (unsigned long i = 0; i < length; ++i)
    {
      _is.read(reinterpret_cast<char *>(data + i), type_size);
    }
  }

  template <typename T>
  static inline void
  Read(T * data, unsigned long length, std::istream & _is)
  {
    if (!data || length < 1)
      return;
    const unsigned int type_size = sizeof(T);
    for (unsigned long i = 0; i < length; ++i)
    {
      _is.read(reinterpret_cast<char *>(data + i), type_size);
    }
    SwapperNoOp::SwapArray(data, length);
  }

  template <typename T>
  static inline void
  Write(const T * data, unsigned long length, std::ostream & _os)
  {
    if (!data || length < 1)
      return;
    const unsigned int type_size = sizeof(T);
    for (unsigned long i = 0; i < length; ++i)
    {
      const T swappedData = SwapperNoOp::Swap(data[i]);
      _os.write(reinterpret_cast<const char *>(&swappedData), type_size);
    }
  }

  template <typename T>
  static inline void
  ReadNoSwapOne(T & data, unsigned long, std::istream & _is)
  {
    const unsigned int type_size = sizeof(T);
    char * cdata = reinterpret_cast<char *>(&data);
    _is.read(cdata, type_size);
  }

  template <typename T>
  static inline void
  ReadOne(T & data, unsigned long, std::istream & _is)
  {
    const unsigned int type_size = sizeof(T);
    char * cdata = reinterpret_cast<char *>(&data);
    _is.read(cdata, type_size);
    void * vdata = static_cast<void*>(cdata);
    SwapperNoOp::SwapArray(static_cast<T*>(vdata), 1);
  }

  template <typename T>
  static inline void
  WriteOne(const T & data, unsigned long, std::ostream & _os)
  {
    const unsigned int type_size = sizeof(T);
    const T swappedData = SwapperNoOp::Swap(data);
    _os.write(reinterpret_cast<const char *>(&swappedData), type_size);
  }
};

// Implementation for the undefined length (dynamically allocated array)
template <long long TVR>
class Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM1_n>)
  };

public:
  Element() = default;

  ~Element()
  {
    if (Save)
    {
      delete[] Internal;
    }
    Internal = nullptr;
  }

  static VR
  GetVR()
  {
    return static_cast<VR::VRType>(TVR);
  }

  static VM
  GetVM()
  {
    return VM::VM1_n;
  }

  // SetLength should be protected anyway, all operation
  // should go through SetArray
  unsigned long
  GetLength() const
  {
    return Length;
  }
  typedef typename VRToType<TVR>::Type Type;

  void
  SetLength(unsigned long len)
  {
    const unsigned int size = sizeof(Type);
    if (len)
    {
      if (len > Length)
      {
        assert((len / size) * size == len);
        Type * i;
        try
        {
          i = new Type[len / size];
        }
        catch (const std::bad_alloc &)
        {
          return;
        }
        assert(Save == false);
        Save = true; //
        if (Internal)
        {
          memcpy(i, Internal, len);
          delete[] Internal;
        }
        Internal = i;
      }
    }
    Length = len / size;
  }

  void
  SetArray(const Type * array, unsigned long len, bool save = false)
  {
    if (save)
    {
      SetLength(len);
      memcpy(Internal, array, len);
      assert(Save == false);
    }
    else
    {
      assert(Length == 0);
      assert(Internal == nullptr);
      assert(Save == false);
      Length = len / sizeof(Type);
      if ((len / sizeof(Type)) * sizeof(Type) != len)
      {
        Internal = nullptr;
        Length = 0;
      }
      else
      {
        Internal = const_cast<Type *>(array);
      }
    }
    Save = save;
  }

  void
  SetValue(typename VRToType<TVR>::Type v, unsigned int idx = 0)
  {
    assert(idx < Length);
    Internal[idx] = v;
  }

  const typename VRToType<TVR>::Type &
  GetValue(unsigned int idx = 0) const
  {
    assert(idx < Length);
    return Internal[idx];
  }

  typename VRToType<TVR>::Type &
  GetValue(unsigned int idx = 0)
  {
    return Internal[idx];
  }

  typename VRToType<TVR>::Type operator[](unsigned int idx) const
  {
    return GetValue(idx);
  }

  void
  Set(const Value & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    if (static_cast<VR::VRType>(VRToEncoding<TVR>::Mode) == VR::VRBINARY)
    {
      const Type * array = static_cast<const Type *>(bv->GetVoidPointer());
      if (array)
      {
        assert(Internal == nullptr);
        SetArray(array, bv->GetLength());
      }
    }
    else
    {
      std::stringstream ss;
      std::string       s = std::string(bv->GetPointer(), bv->GetLength());
      ss.str(s);
      EncodingImplementation<VRToEncoding<TVR>::Mode>::Read(Internal, GetLength(), ss);
    }
  }

  void
  SetFromDataElement(const DataElement & de)
  {
    const ByteValue * bv = de.GetByteValue();
    if (!bv)
      return;
#ifdef MDCM_WORDS_BIGENDIAN
    if (de.GetVR() == VR::UN)
#else
    if (de.GetVR() == VR::UN || de.GetVR() == VR::INVALID)
#endif
    {
      Set(de.GetValue());
    }
    else
    {
      SetNoSwap(de.GetValue());
    }
  }

  // Need to be placed after definition of EncodingImplementation<VR::VRASCII>
  void
  WriteASCII(std::ostream & os) const
  {
    return EncodingImplementation<VR::VRASCII>::Write(Internal, GetLength(), os);
  }

  // Implementation of Print is common to all Mode (ASCII/Binary)
  void
  Print(std::ostream & _os) const
  {
    if (!Internal || Length < 1)
      return;
    _os << Internal[0];
    if (Length > 1)
    {
      const unsigned int l = Length < 25 ? Length : 25; //
      for (unsigned int i = 1; i < l; ++i)
        _os << ',' << Internal[i];
    }
  }

  void
  Read(std::istream & _is)
  {
    if (!Internal)
      return;
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Read(Internal, GetLength(), _is);
  }

  void
  Write(std::ostream & _os) const
  {
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetLength(), _os);
  }

  DataElement
  GetAsDataElement() const
  {
    DataElement ret;
    ret.SetVR(static_cast<VR::VRType>(TVR));
    assert(ret.GetVR() != VR::SQ);
    if (Internal)
    {
      std::ostringstream os;
      EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetLength(), os);
      if (static_cast<VR::VRType>(VRToEncoding<TVR>::Mode) == VR::VRASCII)
      {
        if (GetVR() != VR::UI)
        {
          if (os.str().size() % 2 != 0)
          {
            os << ' ';
          }
        }
      }
      VL::Type osStrSize = static_cast<VL::Type>(os.str().size());
      ret.SetByteValue(os.str().c_str(), osStrSize);
    }
    return ret;
  }

  Element(const Element & _val)
  {
    if (this != &_val)
    {
      *this = _val;
    }
  }

  Element &
  operator=(const Element & _val)
  {
    if (Length > 0 && Internal != nullptr)
    {
      // TODO check delete
      Internal = nullptr;
      Length = 0;
    }
    SetArray(_val.Internal, _val.Length, true);
    return *this;
  }

protected:
  void
  SetNoSwap(const Value & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    if (static_cast<VR::VRType>(VRToEncoding<TVR>::Mode) == VR::VRBINARY)
    {
      const Type * array = static_cast<const Type *>(bv->GetVoidPointer());
      if (array)
      {
        assert(Internal == nullptr);
        SetArray(array, bv->GetLength());
      }
    }
    else
    {
      std::stringstream ss;
      std::string       s = std::string(bv->GetPointer(), bv->GetLength());
      ss.str(s);
      EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadNoSwap(Internal, GetLength(), ss);
    }
  }

private:
  typename VRToType<TVR>::Type * Internal{};
  unsigned long                  Length{};
  bool                           Save{};
};

template <long long TVR>
class Element<TVR, VM::VM1_2> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 2))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_3> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 3))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_4> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 4))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_5> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 5))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_8> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 8))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_32> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 32))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM1_99> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 1 && len <= 99))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM2_4> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 2 && len <= 4))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM3_4> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 3 && len <= 4))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM4_5> : public Element<TVR, VM::VM1_n>
{
public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 4 && len <= 5))
      return;
    Parent::SetLength(len);
  }
};

// Derivatives of 1-n

template <long long TVR>
class Element<TVR, VM::VM2_n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM2_n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (len < 2)
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM2_2n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM2_2n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 2 && len % 2 == 0))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM3_3n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM3_3n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 3 && len % 3 == 0))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM3_n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM3_n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (len < 3)
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM4_4n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM4_4n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 4 && len % 4 == 0))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM6_6n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM6_6n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (!(len >= 6 && len % 6 == 0))
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM6_n> : public Element<TVR, VM::VM1_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM6_n>)
  };

public:
  typedef Element<TVR, VM::VM1_n> Parent;
  void
  SetLength(int len)
  {
    if (len < 6)
      return;
    Parent::SetLength(len);
  }
};

// Skipped VM7_7n, VM30_30n, VM47_47n, not in public dictionary

template <>
class Element<VR::OB, VM::VM1> : public Element<VR::OB, VM::VM1_n>
{};

template <>
class Element<VR::OW, VM::VM1> : public Element<VR::OW, VM::VM1_n>
{};

template <>
class Element<VR::OL, VM::VM1> : public Element<VR::OL, VM::VM1_n>
{};

template <>
class Element<VR::OD, VM::VM1> : public Element<VR::OD, VM::VM1_n>
{};

template <>
class Element<VR::OF, VM::VM1> : public Element<VR::OF, VM::VM1_n>
{};

template <>
class Element<VR::OV, VM::VM1> : public Element<VR::OV, VM::VM1_n>
{};

} // namespace mdcm

#endif // MDCMELEMENT_H
