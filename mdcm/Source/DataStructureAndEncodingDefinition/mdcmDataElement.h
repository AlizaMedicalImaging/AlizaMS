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
#ifndef MDCMDATAELEMENT_H
#define MDCMDATAELEMENT_H

#include "mdcmTag.h"
#include "mdcmVL.h"
#include "mdcmVR.h"
#include "mdcmByteValue.h"
#include "mdcmSmartPointer.h"
#include <set>

namespace mdcm_ns
{
// Data Element
// Contains multiple fields:
// -> Tag
// -> Optional VR (Explicit Transfer Syntax)
// -> ValueLength
// -> Value

class SequenceOfItems;
class SequenceOfFragments;
/**
 * \brief Class to represent a Data Element
 * either Implicit or Explicit
 *
 * \details
 * DATA ELEMENT:
 * A unit of information as defined by a single entry in the data dictionary.
 * An encoded Information Object Definition (IOD) Attribute that is composed
 * of, at a minimum, three fields: a Data Element Tag, a Value Length, and a
 * Value Field. For some specific Transfer Syntaxes, a Data Element also
 * contains a VR Field where the Value Representation of that Data Element is
 * specified explicitly.
 *
 * Design:
 * \li A DataElement in MDCM always store VL (Value Length) on a 32 bits integer even when VL is 16 bits
 * \li A DataElement always store the VR even for Implicit TS, in which case VR is defaulted to VR::INVALID
 * \li For Item start/end (See 0xfffe tags), Value is NULL
 *
 * \see ExplicitDataElement ImplicitDataElement
 */
class MDCM_EXPORT DataElement
{
public:
  DataElement(
    const Tag & t  = Tag(0),
    const VL  & vl = 0,
    const VR  & vr = VR::INVALID)
    :
    TagField(t),
    ValueLengthField(vl),
    VRField(vr),
    ValueField(0) {}

  friend std::ostream& operator<<(std::ostream &, const DataElement &);

  const Tag& GetTag() const { return TagField; }
  Tag& GetTag() { return TagField; }

  // Use with cautious (need to match Part 6)
  void SetTag(const Tag &t) { TagField = t; }

  const VL & GetVL() const { return ValueLengthField; }
  VL& GetVL() { return ValueLengthField; }

  // Use with cautious (need to match Part 6), advanced user only
  void SetVL(const VL & vl) { ValueLengthField = vl; }
  void SetVLToUndefined();

  // Do not set VR::SQ on bytevalue data element
  VR const & GetVR() const { return VRField; }

  // Set VR (not a dual one such as OB_OW)
  void SetVR(VR const & vr)
  {
    if(vr.IsVRFile()) VRField = vr;
  }

  Value const &GetValue() const { mdcmAssertAlwaysMacro(ValueField); return *ValueField; }
  Value & GetValue() { return *ValueField; }

  void SetValue(Value const & vl)
  {
    ValueField = vl;
    ValueLengthField = vl.GetLength();
  }

  bool IsEmpty() const { return ValueField == 0 || (GetByteValue() && GetByteValue()->IsEmpty()); }
  void Empty() { ValueField = 0; ValueLengthField = 0; }

  void Clear()
  {
    TagField = 0;
    VRField = VR::INVALID;
    ValueField = 0;
    ValueLengthField = 0;
  }

  /// Set the byte value
  /// \warning user need to read DICOM standard for an understanding of:
  /// * even padding
  /// * \0 vs space padding
  /// By default even padding is achieved using \0 regardless of the of VR
  void SetByteValue(const char * array, VL length)
  {
    ByteValue * bv = new ByteValue(array,length);
    SetValue(*bv);
  }

  const ByteValue * GetByteValue() const
  {
    const ByteValue * bv = dynamic_cast<const ByteValue*>(ValueField.GetPointer());
    return bv;
  }

  /// Interpret the Value stored in the DataElement. This is more robust (but also more
  /// expensive) to call this function rather than the simpliest form: GetSequenceOfItems()
  /// It also return NULL when the Value is NOT of type SequenceOfItems
  /// \warning in case GetSequenceOfItems() succeed the function return this value, otherwise
  /// it creates a new SequenceOfItems, you should handle that in your case, for instance:
  /// SmartPointer<SequenceOfItems> sqi = de.GetValueAsSQ();
  SmartPointer<SequenceOfItems> GetValueAsSQ() const;

  /// Return the Value of DataElement as a Sequence Of Fragments (if possible)
  /// \warning: You need to check for NULL return value
  const SequenceOfFragments * GetSequenceOfFragments() const;
  SequenceOfFragments * GetSequenceOfFragments();

  bool IsUndefinedLength() const
  {
    return ValueLengthField.IsUndefined();
  }

  DataElement(const DataElement & _val)
  {
    if(this != &_val)
    {
      *this = _val;
    }
  }

  bool operator<(const DataElement & de) const
  {
    return GetTag() < de.GetTag();
  }

  DataElement &operator=(const DataElement & de)
  {
    TagField = de.TagField;
    ValueLengthField = de.ValueLengthField;
    VRField = de.VRField;
    ValueField = de.ValueField;
    return *this;
  }

  bool operator==(const DataElement & de) const
  {
    bool b = TagField == de.TagField
      && ValueLengthField == de.ValueLengthField
      && VRField == de.VRField;
    if(!ValueField && !de.ValueField)
    {
      return b;
    }
    if(ValueField && de.ValueField)
    {
      return b && (*ValueField == *de.ValueField);
    }
    return false;
  }

  // The following functionalities are dependant on:
  //   Transfer Syntax: Explicit or Implicit
  //   The Byte encoding: Little Endian / Big Endian

  /*
   * The following was inspired by a C++ idiom: Curiously Recurring Template Pattern
   * Ref: http://en.wikipedia.org/wiki/Curiously_Recurring_Template_Pattern
   * The typename TDE is typically a derived class *without* any data
   * while TSwap is a simple template parameter to achieve byteswapping (and allow factorization of
   * highly identical code)
   */
  template <typename TDE>
  VL GetLength() const
  {
    return static_cast<const TDE*>(this)->GetLength();
  }

  template <typename TDE, typename TSwap>
  std::istream & Read(std::istream & is)
  {
    return static_cast<TDE*>(this)->template Read<TSwap>(is);
  }

  template <typename TDE, typename TSwap>
  std::istream & ReadOrSkip(std::istream & is, std::set<Tag> const & skiptags)
  {
    (void)skiptags;
    return static_cast<TDE*>(this)->template Read<TSwap>(is);
  }

  template <typename TDE, typename TSwap>
  std::istream & ReadPreValue(std::istream & is, std::set<Tag> const & skiptags)
  {
    (void)skiptags;
    return static_cast<TDE*>(this)->template ReadPreValue<TSwap>(is);
  }
  template <typename TDE, typename TSwap>
  std::istream & ReadValue(std::istream & is, std::set<Tag> const & skiptags)
  {
    (void)skiptags;
    return static_cast<TDE*>(this)->template ReadValue<TSwap>(is);
  }
  template <typename TDE, typename TSwap>
  std::istream & ReadValueWithLength(std::istream & is, VL & length, std::set<Tag> const & skiptags)
  {
    (void)skiptags;
    return static_cast<TDE*>(this)->template ReadValueWithLength<TSwap>(is, length);
  }

  template <typename TDE, typename TSwap>
  std::istream & ReadWithLength(std::istream & is, VL & length)
  {
    return static_cast<TDE*>(this)->template ReadWithLength<TSwap>(is, length);
  }

  template <typename TDE, typename TSwap>
  const std::ostream & Write(std::ostream & os) const
  {
    return static_cast<const TDE*>(this)->template Write<TSwap>(os);
  }

protected:
  Tag TagField;
  // This is the value read from the file, might be different from the length of ValueField
  VL ValueLengthField; // Can be 0xFFFFFFFF
  VR VRField;
  typedef SmartPointer<Value> ValuePtr;
  ValuePtr ValueField;
  void SetValueFieldLength(VL vl, bool readvalues);
};

inline std::ostream& operator<<(std::ostream & os, const DataElement & val)
{
  os << val.TagField;
  os << "\t" << val.VRField;
  os << "\t" << val.ValueLengthField;
  if(val.ValueField)
  {
    val.ValueField->Print(os << "\t");
  }
  return os;
}

inline bool operator!=(const DataElement & lhs, const DataElement & rhs)
{
  return !(lhs == rhs);
}

} // end namespace mdcm_ns

#endif //MDCMDATAELEMENT_H
