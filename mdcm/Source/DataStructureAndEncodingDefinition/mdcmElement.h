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
    return (VR::VRType)TVR;
  }

  static VM
  GetVM()
  {
    return (VM::VMType)TVM;
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
  SetFromDataElement(DataElement const & de)
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
    ret.SetVR((VR::VRType)TVR);
    assert(ret.GetVR() != VR::SQ);
    if ((VR::VRType)VRToEncoding<TVR>::Mode == VR::VRASCII)
    {
      if (GetVR() != VR::UI)
      {
        if (os.str().size() % 2)
        {
          os << " ";
        }
      }
    }
    VL::Type osStrSize = (VL::Type)os.str().size();
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
  Set(Value const & v)
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
  SetNoSwap(Value const & v)
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
operator>>(std::istream & in, ignore_char const & ic)
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
      ;;
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
        _os << "\\" << data[i];
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

//#define VRDS16ILLEGAL
#ifdef VRDS16ILLEGAL
template <typename Float>
std::string
to_string(Float data)
{
  std::stringstream in;
  int const         digits =
    static_cast<int>(-std::log(std::numeric_limits<Float>::epsilon()) / static_cast<Float>(std::log(10.0)));
  if (in << std::dec << std::setprecision(digits) << data)
  {
    return (in.str());
  }
  else
  {
    throw std::logic_error("Impossible conversion");
  }
}
#else
// http://stackoverflow.com/questions/32631178/writing-ieee-754-1985-double-as-ascii-on-a-limited-16-bytes-string
static inline void
clean(char * mant)
{
  char * ix = mant + strlen(mant) - 1;
  while (('0' == *ix) && (ix > mant))
  {
    *ix-- = '\0';
  }
  if ('.' == *ix)
  {
    *ix = '\0';
  }
}

static int
add1(char * buf, int n)
{
  if (n < 0)
    return 1;
  if (buf[n] == '9')
  {
    buf[n] = '0';
    return add1(buf, n - 1);
  }
  else
  {
    buf[n] = (char)(buf[n] + 1);
  }
  return 0;
}

static int
doround(char * buf, unsigned int n)
{
  char c;
  if (n >= strlen(buf))
    return 0;
  c = buf[n];
  buf[n] = 0;
  if ((c >= '5') && (c <= '9'))
    return add1(buf, n - 1);
  return 0;
}

static int
roundat(char * buf, unsigned int i, int iexp)
{
  if (doround(buf, i) != 0)
  {
    iexp += 1;
    switch (iexp)
    {
      case -2:
        strcpy(buf, ".01");
        break;
      case -1:
        strcpy(buf, ".1");
        break;
      case 0:
        strcpy(buf, "1.");
        break;
      case 1:
        strcpy(buf, "10");
        break;
      case 2:
        strcpy(buf, "100");
        break;
      default:
        sprintf(buf, "1e%d", iexp);
    }
    return 1;
  }
  return 0;
}

template <typename Float>
static void
x16printf(char * buf, int size, Float f)
{
  char line[40];
  char * mant = line + 1;
  int iexp, lexp, i;
  char exp[6];
  if (f < 0)
  {
    f = -f;
    size -= 1;
    *buf++ = '-';
  }
  sprintf(line, "%1.16e", f);
  if (line[0] == '-')
  {
    f = -f;
    size -= 1;
    *buf++ = '-';
    sprintf(line, "%1.16e", f);
  }
  *mant = line[0];
  i = (int)strcspn(mant, "eE");
  mant[i] = '\0';
  iexp = (int)strtol(mant + i + 1, NULL, 10);
  lexp = sprintf(exp, "e%d", iexp);
  if ((iexp >= size) || (iexp < -3))
  {
    i = roundat(mant, size - 1 - lexp, iexp);
    if (i == 1)
    {
      strcpy(buf, mant);
      return;
    }
    buf[0] = mant[0];
    buf[1] = '.';
    strncpy(buf + i + 2, mant + 1, size - 2 - lexp);
    buf[size - lexp] = 0;
    clean(buf);
    strcat(buf, exp);
  }
  else if (iexp >= size - 2)
  {
    roundat(mant, iexp + 1, iexp);
    strcpy(buf, mant);
  }
  else if (iexp >= 0)
  {
    i = roundat(mant, size - 1, iexp);
    if (i == 1)
    {
      strcpy(buf, mant);
      return;
    }
    strncpy(buf, mant, iexp + 1);
    buf[iexp + 1] = '.';
    strncpy(buf + iexp + 2, mant + iexp + 1, size - iexp - 1);
    buf[size] = 0;
    clean(buf);
  }
  else
  {
    i = roundat(mant, size + 1 + iexp, iexp);
    if (i == 1)
    {
      strcpy(buf, mant);
      return;
    }
    buf[0] = '.';
    for (int j = 0; j < -1 - iexp; ++j)
    {
      buf[j + 1] = '0';
    }
    /* FIXME dead code
        if ((i == 1) && (iexp != -1))
        {
          buf[-iexp] = '1';
          buf++;
        }
    */
    strncpy(buf - iexp, mant, size + 1 + iexp);
    buf[size] = 0;
    clean(buf);
  }
}
#endif

template <>
inline void
EncodingImplementation<VR::VRASCII>::Write(const double * data, unsigned long length, std::ostream & _os)
{
  if (!data || length < 1)
    return;
#ifdef VRDS16ILLEGAL
  _os << to_string(data[0]);
#else
  char buf[16 + 1];
  x16printf(buf, 16, data[0]);
  _os << buf;
#endif
  if (length > 1)
  {
    for (unsigned long i = 1; i < length; ++i)
    {
#ifdef VRDS16ILLEGAL
      _os << "\\" << to_string(data[i]);
#else
      x16printf(buf, 16, data[i]);
      _os << "\\" << buf;
#endif
    }
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
    SwapperNoOp::SwapArray((T*)cdata, 1);
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
  explicit Element()
  {
    Internal = NULL;
    Length = 0;
    Save = false;
  }

  ~Element()
  {
    if (Save)
    {
      if (Internal)
        delete[] Internal;
    }
    Internal = NULL;
  }

  static VR
  GetVR()
  {
    return (VR::VRType)TVR;
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
        Type * internal;
        try
        {
          internal = new Type[len / size];
        }
        catch (std::bad_alloc &)
        {
          return;
        }
        assert(Save == false);
        Save = true; //
        if (Internal)
        {
          memcpy(internal, Internal, len);
          delete[] Internal;
        }
        Internal = internal;
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
      assert(Internal == NULL);
      assert(Save == false);
      Length = len / sizeof(Type);
      if ((len / sizeof(Type)) * sizeof(Type) != len)
      {
        Internal = NULL;
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
  Set(Value const & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    if ((VR::VRType)(VRToEncoding<TVR>::Mode) == VR::VRBINARY)
    {
      const Type * array = (const Type *)bv->GetVoidPointer();
      if (array)
      {
        assert(Internal == NULL);
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
  SetFromDataElement(DataElement const & de)
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
      const unsigned long length = Length < 25 ? Length : 25; //
      for (unsigned long i = 1; i < length; ++i)
        _os << "," << Internal[i];
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
    ret.SetVR((VR::VRType)TVR);
    assert(ret.GetVR() != VR::SQ);
    if (Internal)
    {
      std::ostringstream os;
      EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetLength(), os);
      if ((VR::VRType)VRToEncoding<TVR>::Mode == VR::VRASCII)
      {
        if (GetVR() != VR::UI)
        {
          if (os.str().size() % 2)
          {
            os << " ";
          }
        }
      }
      VL::Type osStrSize = (VL::Type)os.str().size();
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
    if (Length > 0 && Internal != NULL)
    {
      // TODO check delete
      Internal = NULL;
      Length = 0;
    }
    SetArray(_val.Internal, _val.Length, true);
    return *this;
  }

protected:
  void
  SetNoSwap(Value const & v)
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(&v);
    if (!bv)
      return;
    if ((VR::VRType)(VRToEncoding<TVR>::Mode) == VR::VRBINARY)
    {
      const Type * array = (const Type *)bv->GetVoidPointer();
      if (array)
      {
        assert(Internal == NULL);
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
  typename VRToType<TVR>::Type * Internal;
  unsigned long                  Length;
  bool                           Save;
};

// Partial specialization for derivatives of 1-n : 2-n, 3-n
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
    if (len <= 1)
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM2_2n> : public Element<TVR, VM::VM2_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM2_2n>)
  };

public:
  typedef Element<TVR, VM::VM2_n> Parent;
  void
  SetLength(int len)
  {
    if (len % 2)
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
    if (len <= 2)
      return;
    Parent::SetLength(len);
  }
};

template <long long TVR>
class Element<TVR, VM::VM3_3n> : public Element<TVR, VM::VM3_n>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<TVR, VM::VM3_3n>)
  };

public:
  typedef Element<TVR, VM::VM3_n> Parent;
  void
  SetLength(int len)
  {
    if (len % 3)
      return;
    Parent::SetLength(len);
  }
};

template <>
class Element<VR::AS, VM::VM5>
{
  enum
  {
    ElementDisableCombinationsCheck = sizeof(ElementDisableCombinations<VR::AS, VM::VM5>)
  };

public:
  char Internal[VMToLength<VM::VM5>::Length * sizeof(VRToType<VR::AS>::Type)];
  void
  Print(std::ostream & _os) const
  {
    _os << Internal;
  }

  unsigned long
  GetLength() const
  {
    return VMToLength<VM::VM5>::Length;
  }
};

template <>
class Element<VR::OB, VM::VM1> : public Element<VR::OB, VM::VM1_n>
{};

template <>
class Element<VR::OW, VM::VM1> : public Element<VR::OW, VM::VM1_n>
{};

} // namespace mdcm

#endif // MDCMELEMENT_H
