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
#ifndef MDCMCSAELEMENT_H
#define MDCMCSAELEMENT_H

#include "mdcmTag.h"
#include "mdcmVM.h"
#include "mdcmVR.h"
#include "mdcmByteValue.h"
#include "mdcmSmartPointer.h"

namespace mdcm
{
/**
 * Class to represent a CSA Element
 */
class MDCM_EXPORT CSAElement
{
  friend std::ostream &
  operator<<(std::ostream &, const CSAElement &);

public:
  CSAElement() = default;

  explicit CSAElement(unsigned int kf)
    : KeyField(kf)
  {}

  unsigned int
  GetKey() const
  {
    return KeyField;
  }

  void
  SetKey(unsigned int key)
  {
    KeyField = key;
  }

  const char *
  GetName() const
  {
    return NameField.c_str();
  }

  void
  SetName(const char * name)
  {
    NameField = name;
  }

  const VM &
  GetVM() const
  {
    return ValueMultiplicityField;
  }

  void
  SetVM(const VM & vm)
  {
    ValueMultiplicityField = vm;
  }

  const VR &
  GetVR() const
  {
    return VRField;
  }

  void
  SetVR(const VR & vr)
  {
    VRField = vr;
  }

  unsigned int
  GetSyngoDT() const
  {
    return SyngoDTField;
  }

  void
  SetSyngoDT(unsigned int syngodt)
  {
    SyngoDTField = syngodt;
  }

  unsigned int
  GetNoOfItems() const
  {
    return NoOfItemsField;
  }

  void
  SetNoOfItems(unsigned int items)
  {
    NoOfItemsField = items;
  }

  // Set/Get Value (bytes array, SQ of items, SQ of fragments):
  const Value &
  GetValue() const
  {
    return *DataField; // Always check IsEmpty() before!
  }

  Value &
  GetValue()
  {
    return *DataField; // Always check IsEmpty() before!
  }

  void
  SetValue(const Value & vl)
  {
    DataField = vl;
  }

  bool
  IsEmpty() const
  {
    return DataField == nullptr;
  }

  void
  SetByteValue(const char * array, VL length)
  {
    ByteValue * bv = new ByteValue(array, length);
    SetValue(*bv);
  }

  // Return the Value of CSAElement as a ByteValue (if possible),
  // check for nullptr return value
  const ByteValue *
  GetByteValue() const
  {
    const ByteValue * bv = dynamic_cast<const ByteValue *>(DataField.GetPointer());
    return bv; // Will return nullptr if not ByteValue
  }

  bool
  operator<(const CSAElement & de) const
  {
    return GetKey() < de.GetKey();
  }

  CSAElement &
  operator=(const CSAElement & de)
  {
    KeyField = de.KeyField;
    NameField = de.NameField;
    ValueMultiplicityField = de.ValueMultiplicityField;
    VRField = de.VRField;
    SyngoDTField = de.SyngoDTField;
    NoOfItemsField = de.NoOfItemsField;
    DataField = de.DataField; // Pointer copy
    return *this;
  }

  bool
  operator==(const CSAElement & de) const
  {
    return (KeyField == de.KeyField && NameField == de.NameField &&
            ValueMultiplicityField == de.ValueMultiplicityField && VRField == de.VRField &&
            SyngoDTField == de.SyngoDTField);
  }

protected:
  unsigned int                KeyField{};
  std::string                 NameField{};
  VM                          ValueMultiplicityField{};
  VR                          VRField{};
  unsigned int                SyngoDTField{};
  unsigned int                NoOfItemsField{};
  SmartPointer<Value>         DataField{};
};

inline std::ostream &
operator<<(std::ostream & os, const CSAElement & val)
{
  os << val.KeyField;
  os << ") " << val.NameField << "  ";
  if (val.DataField)
  {
    const ByteValue * bv = dynamic_cast<ByteValue *>(&*val.DataField);
    if (!bv)
      return os;
    std::string str(bv->GetPointer(), bv->GetLength());
    if (val.ValueMultiplicityField == VM::VM1)
    {
      os << str.c_str();
    }
    else
    {
      std::istringstream is(str);
      std::string        s;
      bool               sep = false;
      while (std::getline(is, s, '\\'))
      {
        if (sep)
          os << '\\';
        sep = true;
        os << s.c_str();
      }
    }
  }
  return os;
}

} // end namespace mdcm

#endif // MDCMCSAELEMENT_H
