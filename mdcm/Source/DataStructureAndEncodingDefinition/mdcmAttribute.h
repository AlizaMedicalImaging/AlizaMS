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
#ifndef MDCMATTRIBUTE_H
#define MDCMATTRIBUTE_H

#include "mdcmTypes.h"
#include "mdcmVR.h"
#include "mdcmTagToType.h"
#include "mdcmVM.h"
#include "mdcmElement.h"
#include "mdcmDataElement.h"
#include "mdcmDataSet.h"
#include "mdcmStaticAssert.h"
#include <string>
#include <vector>
#include <sstream>

namespace mdcm
{

#if 0
template <int T>
class VRVLSize;

// VL is coded on 16 bits
template <>
class VRVLSize<0>
{
public:
  static inline uint16_t
  Read(std::istream & _is)
  {
    uint16_t l;
    _is.read((char *)&l, 2);
    return l;
  }
  static inline void
  Write(std::ostream & os)
  {
    (void)os;
  }
};

// VL is coded on 32 bits
template <>
class VRVLSize<1>
{
public:
  static inline uint32_t
  Read(std::istream & _is)
  {
    char dummy[2];
    _is.read(dummy, 2);
    uint32_t l;
    _is.read((char *)&l, 4);
    return l;
  }
  static inline void
  Write(std::ostream & os)
  {
    (void)os;
  }
};
#endif

/**
 * Attribute class
 * This class use template metaprograming tricks to let the user know when the template
 * instanciation does not match the public dictionary.
 *
 * Typical example that compile is:
 * Attribute<0x0008,0x9007> a = {"ORIGINAL","PRIMARY","T1","NONE"};
 *
 * Examples that will NOT compile are:
 *
 * Attribute<0x0018,0x1182, VR::IS, VM::VM1> fd1 = {}; // not enough parameters
 * Attribute<0x0018,0x1182, VR::IS, VM::VM2> fd2 = {0,1,2}; // too many initializers
 */

template <uint16_t  Group,
          uint16_t  Element,
          long long TVR = TagToType<Group, Element>::VRType,
          int       TVM = TagToType<Group, Element>::VMType>
class Attribute
{
public:
  typedef typename VRToType<TVR>::Type ArrayType;
  enum
  {
    VMType = VMToLength<TVM>::Length
  };

  ArrayType Internal[VMToLength<TVM>::Length];
  // VR/VM must be compatible with the public dictionary
  MDCM_STATIC_ASSERT(((VR::VRType)TVR & (VR::VRType)(TagToType<Group, Element>::VRType)));
  MDCM_STATIC_ASSERT(((VM::VMType)TVM & (VM::VMType)(TagToType<Group, Element>::VMType)));
  MDCM_STATIC_ASSERT(((((VR::VRType)TVR & VR::VR_VM1) && ((VM::VMType)TVM == VM::VM1)) ||
                      !((VR::VRType)TVR & VR::VR_VM1)));

  static Tag
  GetTag()
  {
    return Tag(Group, Element);
  }

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

  // The following two methods do make sense only in case of public element,
  // when the template is intanciated with private element the VR/VM are simply
  // defaulted to allow everything (see mdcmTagToType.h default template for TagToType)
  static VR
  GetDictVR()
  {
    return (VR::VRType)(TagToType<Group, Element>::VRType);
  }

  static VM
  GetDictVM()
  {
    return (VM::VMType)(TagToType<Group, Element>::VMType);
  }

  unsigned int
  GetNumberOfValues() const
  {
    return VMToLength<TVM>::Length;
  }

  void
  Print(std::ostream & os) const
  {
    os << GetTag() << " ";
    os << TagToType<Group, Element>::GetVRString() << " ";
    os << TagToType<Group, Element>::GetVMString() << " ";
    os << Internal[0];
    for (unsigned int i = 1; i < GetNumberOfValues(); ++i)
      os << "," << Internal[i];
  }

  bool
  operator==(const Attribute & att) const
  {
    return std::equal(Internal, Internal + GetNumberOfValues(), att.GetValues());
  }

  bool
  operator!=(const Attribute & att) const
  {
    return !std::equal(Internal, Internal + GetNumberOfValues(), att.GetValues());
  }

  bool
  operator<(const Attribute & att) const
  {
    return std::lexicographical_compare(
      Internal, Internal + GetNumberOfValues(), att.GetValues(), att.GetValues() + att.GetNumberOfValues());
  }

  ArrayType &
  GetValue(unsigned int idx = 0)
  {
    assert(idx < GetNumberOfValues());
    return Internal[idx];
  }

  ArrayType & operator[](unsigned int idx) { return GetValue(idx); }

  ArrayType const &
  GetValue(unsigned int idx = 0) const
  {
    assert(idx < GetNumberOfValues());
    return Internal[idx];
  }

  ArrayType const & operator[](unsigned int idx) const { return GetValue(idx); }

  void
  SetValue(ArrayType v, unsigned int idx = 0)
  {
    assert(idx < GetNumberOfValues());
    Internal[idx] = v;
  }

  void
  SetValues(const ArrayType * array, unsigned int numel = VMType)
  {
    assert(array && numel && numel == GetNumberOfValues());
    std::copy(array, array + numel, Internal);
  }

  const ArrayType *
  GetValues() const
  {
    return Internal;
  }

  DataElement
  GetAsDataElement() const
  {
    DataElement        ret(GetTag());
    std::ostringstream os;
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetNumberOfValues(), os);
    ret.SetVR(GetVR());
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
  SetFromDataElement(DataElement const & de)
  {
    assert(GetTag() == de.GetTag() || GetTag().GetGroup() == 0x6000 || GetTag().GetGroup() == 0x5000); // ?
    assert(GetVR() != VR::INVALID);
    assert(GetVR().Compatible(de.GetVR()) ||
           de.GetVR() == VR::INVALID); // In case of VR::INVALID cannot use the & operator
    if (de.IsEmpty())
      return;
    const ByteValue * bv = de.GetByteValue();
#ifdef MDCM_WORDS_BIGENDIAN
    if (de.GetVR() == VR::UN)
#else
    if (de.GetVR() == VR::UN || de.GetVR() == VR::INVALID)
#endif
    {
      SetByteValue(bv);
    }
    else
    {
      SetByteValueNoSwap(bv);
    }
  }

  void
  Set(DataSet const & ds)
  {
    SetFromDataElement(ds.GetDataElement(GetTag()));
  }

  void
  SetFromDataSet(DataSet const & ds)
  {
    if (ds.FindDataElement(GetTag()) && !ds.GetDataElement(GetTag()).IsEmpty())
    {
      SetFromDataElement(ds.GetDataElement(GetTag()));
    }
  }

protected:
  void
  SetByteValueNoSwap(const ByteValue * bv)
  {
    if (!bv)
      return;
    assert(bv->GetPointer() && bv->GetLength());
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadNoSwap(Internal, GetNumberOfValues(), ss);
  }

  void
  SetByteValue(const ByteValue * bv)
  {
    if (!bv)
      return;
    assert(bv->GetPointer() && bv->GetLength());
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::Read(Internal, GetNumberOfValues(), ss);
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM1>
{
public:
  typedef typename VRToType<TVR>::Type ArrayType;
  enum
  {
    VMType = VMToLength<VM::VM1>::Length
  };
  ArrayType Internal;
  MDCM_STATIC_ASSERT(VMToLength<VM::VM1>::Length == 1);
  // VR/VM must be compatible with the public dictionary
  MDCM_STATIC_ASSERT(((VR::VRType)TVR & (VR::VRType)(TagToType<Group, Element>::VRType)));
  MDCM_STATIC_ASSERT(((VM::VMType)VM::VM1 & (VM::VMType)(TagToType<Group, Element>::VMType)));
  MDCM_STATIC_ASSERT(((((VR::VRType)TVR & VR::VR_VM1) && ((VM::VMType)VM::VM1 == VM::VM1)) ||
                      !((VR::VRType)TVR & VR::VR_VM1)));

  static Tag
  GetTag()
  {
    return Tag(Group, Element);
  }

  static VR
  GetVR()
  {
    return (VR::VRType)TVR;
  }

  static VM
  GetVM()
  {
    return (VM::VMType)VM::VM1;
  }

  // The following two methods do make sense only in case of public element,
  // when the template is intanciated with private element the VR/VM are simply
  // defaulted to allow everything (see mdcmTagToType.h default template for TagToType)
  static VR
  GetDictVR()
  {
    return (VR::VRType)(TagToType<Group, Element>::VRType);
  }

  static VM
  GetDictVM()
  {
    return (VM::VMType)(TagToType<Group, Element>::VMType);
  }

  unsigned int
  GetNumberOfValues() const
  {
    return VMToLength<VM::VM1>::Length;
  }

  void
  Print(std::ostream & os) const
  {
    os << GetTag() << " ";
    os << TagToType<Group, Element>::GetVRString() << " ";
    os << TagToType<Group, Element>::GetVMString() << " ";
    os << Internal;
  }

  bool
  operator==(const Attribute & att) const
  {
    return std::equal(&Internal, &Internal + GetNumberOfValues(), att.GetValues());
  }

  bool
  operator!=(const Attribute & att) const
  {
    return !std::equal(&Internal, &Internal + GetNumberOfValues(), att.GetValues());
  }

  bool
  operator<(const Attribute & att) const
  {
    return std::lexicographical_compare(
      &Internal, &Internal + GetNumberOfValues(), att.GetValues(), att.GetValues() + att.GetNumberOfValues());
  }

  ArrayType &
  GetValue()
  {
    return Internal;
  }

  ArrayType const &
  GetValue() const
  {
    return Internal;
  }

  void
  SetValue(ArrayType v)
  {
    Internal = v;
  }

  const ArrayType *
  GetValues() const
  {
    return &Internal;
  }

  DataElement
  GetAsDataElement() const
  {
    DataElement        ret(GetTag());
    std::ostringstream os;
    EncodingImplementation<VRToEncoding<TVR>::Mode>::WriteOne(Internal, GetNumberOfValues(), os);
    ret.SetVR(GetVR());
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
  SetFromDataElement(DataElement const & de)
  {
    assert(GetTag() == de.GetTag() || GetTag().GetGroup() == 0x6000 || GetTag().GetGroup() == 0x5000); // ?
    assert(GetVR() != VR::INVALID);
    assert(GetVR().Compatible(de.GetVR()) || de.GetVR() == VR::INVALID);
    if (de.IsEmpty())
      return;
    const ByteValue * bv = de.GetByteValue();
#ifdef MDCM_WORDS_BIGENDIAN
    if (de.GetVR() == VR::UN)
#else
    if (de.GetVR() == VR::UN || de.GetVR() == VR::INVALID)
#endif
    {
      SetByteValue(bv);
    }
    else
    {
      SetByteValueNoSwap(bv);
    }
  }

  void
  Set(DataSet const & ds)
  {
    SetFromDataElement(ds.GetDataElement(GetTag()));
  }

  void
  SetFromDataSet(DataSet const & ds)
  {
    if (ds.FindDataElement(GetTag()) && !ds.GetDataElement(GetTag()).IsEmpty())
    {
      SetFromDataElement(ds.GetDataElement(GetTag()));
    }
  }

protected:
  void
  SetByteValueNoSwap(const ByteValue * bv)
  {
    if (!bv)
      return;
    assert(bv->GetPointer() && bv->GetLength());
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadNoSwapOne(Internal, GetNumberOfValues(), ss);
  }

  void
  SetByteValue(const ByteValue * bv)
  {
    if (!bv)
      return;
    assert(bv->GetPointer() && bv->GetLength());
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    ss.str(s);
    EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadOne(Internal, GetNumberOfValues(), ss);
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM1_n>
{
public:
  typedef typename VRToType<TVR>::Type ArrayType;
  // VR/VM must be compatible with the public dictionary
  MDCM_STATIC_ASSERT(((VR::VRType)TVR & (VR::VRType)(TagToType<Group, Element>::VRType)));
  MDCM_STATIC_ASSERT((VM::VM1_n & (VM::VMType)(TagToType<Group, Element>::VMType)));
  MDCM_STATIC_ASSERT(((((VR::VRType)TVR & VR::VR_VM1) && ((VM::VMType)TagToType<Group, Element>::VMType == VM::VM1)) ||
                      !((VR::VRType)TVR & VR::VR_VM1)));

  static Tag
  GetTag()
  {
    return Tag(Group, Element);
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

  static VR
  GetDictVR()
  {
    return (VR::VRType)(TagToType<Group, Element>::VRType);
  }

  static VM
  GetDictVM()
  {
    return GetVM();
  }

  explicit Attribute()
  {
    Internal = NULL;
    Length = 0;
    Own = true;
  }

  ~Attribute()
  {
    if (Own)
    {
      delete[] Internal;
    }
    Internal = NULL;
  }

  unsigned int
  GetNumberOfValues() const
  {
    return Length;
  }

  void
  SetNumberOfValues(unsigned int n)
  {
    SetValues(NULL, n, true);
  }

  const ArrayType *
  GetValues() const
  {
    return Internal;
  }

  void
  Print(std::ostream & os) const
  {
    os << GetTag() << " ";
    os << GetVR() << " ";
    os << GetVM() << " ";
    if (Length > 0)
      os << Internal[0];
    if (Length > 1)
    {
      for (unsigned int i = 1; i < GetNumberOfValues(); ++i)
        os << "," << Internal[i];
    }
  }

  ArrayType &
  GetValue(unsigned int idx = 0)
  {
    assert(idx < GetNumberOfValues());
    return Internal[idx];
  }

  ArrayType & operator[](unsigned int idx) { return GetValue(idx); }

  ArrayType const &
  GetValue(unsigned int idx = 0) const
  {
    assert(idx < GetNumberOfValues());
    return Internal[idx];
  }

  ArrayType const & operator[](unsigned int idx) const { return GetValue(idx); }

  void
  SetValue(unsigned int idx, ArrayType v)
  {
    assert(idx < GetNumberOfValues());
    Internal[idx] = v;
  }

  void
  SetValue(ArrayType v)
  {
    SetValue(0, v);
  }

  void
  SetValues(const ArrayType * array, unsigned int numel, bool own = false)
  {
    if (Internal)
    {
      if (Own)
        delete[] Internal;
      Internal = NULL;
    }
    Own = own;
    Length = numel;
    if (own)
    {
      try
      {
        Internal = new ArrayType[numel];
      }
      catch (std::bad_alloc &)
      {
        return;
      }
      if (array && numel)
      {
        std::copy(array, array + numel, Internal);
      }
    }
    else
    {
      Internal = const_cast<ArrayType *>(array);
    }
    assert(numel == GetNumberOfValues());
  }

  DataElement
  GetAsDataElement() const
  {
    DataElement        ret(GetTag());
    std::ostringstream os;
    if (Internal)
    {
      EncodingImplementation<VRToEncoding<TVR>::Mode>::Write(Internal, GetNumberOfValues(), os);
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
    }
    ret.SetVR(GetVR());
    assert(ret.GetVR() != VR::SQ);
    VL::Type osStrSize = (VL::Type)os.str().size();
    ret.SetByteValue(os.str().c_str(), osStrSize);
    return ret;
  }

  void
  SetFromDataElement(DataElement const & de)
  {
    assert(GetTag() == de.GetTag() || GetTag().GetGroup() == 0x6000 || GetTag().GetGroup() == 0x5000); // ?
    assert(GetVR().Compatible(de.GetVR()));
    assert(!de.IsEmpty());
    const ByteValue * bv = de.GetByteValue();
    SetByteValue(bv);
  }

  void
  Set(DataSet const & ds)
  {
    SetFromDataElement(ds.GetDataElement(GetTag()));
  }

  void
  SetFromDataSet(DataSet const & ds)
  {
    if (ds.FindDataElement(GetTag()) && !ds.GetDataElement(GetTag()).IsEmpty())
    {
      SetFromDataElement(ds.GetDataElement(GetTag()));
    }
  }

protected:
  void
  SetByteValue(const ByteValue * bv)
  {
    if (!bv)
      return;
    std::stringstream ss;
    std::string       s = std::string(bv->GetPointer(), bv->GetLength());
    Length = bv->GetLength(); // overallocation
    ss.str(s);
    ArrayType * internal;
    try
    {
      internal = new ArrayType[(VL::Type)bv->GetLength()]; // overallocation
    }
    catch (std::bad_alloc &)
    {
      return;
    }
    // update length
    EncodingImplementation<VRToEncoding<TVR>::Mode>::ReadComputeLength(internal, Length, ss);
    SetValues(internal, Length, true);
    delete[] internal;
  }

private:
  ArrayType *  Internal;
  unsigned int Length;
  bool         Own;
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM1_3> : public Attribute<Group, Element, TVR, VM::VM1_n>
{
public:
  VM
  GetVM() const
  {
    return VM::VM1_3;
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM1_8> : public Attribute<Group, Element, TVR, VM::VM1_n>
{
public:
  VM
  GetVM() const
  {
    return VM::VM1_8;
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM2_n> : public Attribute<Group, Element, TVR, VM::VM1_n>
{
public:
  VM
  GetVM() const
  {
    return VM::VM2_n;
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM2_2n> : public Attribute<Group, Element, TVR, VM::VM2_n>
{
public:
  static VM
  GetVM()
  {
    return VM::VM2_2n;
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM3_n> : public Attribute<Group, Element, TVR, VM::VM1_n>
{
public:
  static VM
  GetVM()
  {
    return VM::VM3_n;
  }
};

template <uint16_t Group, uint16_t Element, long long TVR>
class Attribute<Group, Element, TVR, VM::VM3_3n> : public Attribute<Group, Element, TVR, VM::VM3_n>
{
public:
  static VM
  GetVM()
  {
    return VM::VM3_3n;
  }
};

} // namespace mdcm

#endif // MDCMATTRIBUTE_H
