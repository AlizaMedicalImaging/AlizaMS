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

#include "mdcmDataElement.h"
#include "mdcmByteValue.h"
#include "mdcmAttribute.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmTrace.h"
#include <typeinfo>

namespace mdcm
{

DataElement::DataElement(const DataElement & _val)
{
  if(this != &_val)
  {
    *this = _val;
  }
}

const Tag & DataElement::GetTag() const
{
  return TagField;
}

Tag & DataElement::GetTag()
{
  return TagField;
}

void DataElement::SetTag(const Tag & t)
{
  TagField = t;
}

const VL & DataElement::GetVL() const
{
  return ValueLengthField;
}

VL & DataElement::GetVL()
{
  return ValueLengthField;
}

void DataElement::SetVL(const VL & vl)
{
  ValueLengthField = vl;
}

void DataElement::SetVLToUndefined()
{
  try
  {
    SequenceOfItems * sqi =
      dynamic_cast<SequenceOfItems*>(ValueField.GetPointer());
    if(sqi) sqi->SetLengthToUndefined();
  }
  catch(std::bad_cast&) { ;; }
  ValueLengthField.SetToUndefined();
}

const VR & DataElement::GetVR() const
{
  return VRField;
}

void DataElement::SetVR(const VR & vr)
{
  if(vr.IsVRFile()) VRField = vr;
}

const Value & DataElement::GetValue() const
{
  return *ValueField;
}

Value & DataElement::GetValue()
{
  return *ValueField;
}

void DataElement::SetValue(Value const & vl)
{
  ValueField = vl;
  ValueLengthField = vl.GetLength();
}

bool DataElement::IsEmpty() const
{
  return (ValueField == 0 || (GetByteValue() && GetByteValue()->IsEmpty()));
}

void DataElement::Empty()
{
  ValueField = 0;
  ValueLengthField = 0;
}

void DataElement::Clear()
{
  TagField = 0;
  VRField = VR::INVALID;
  ValueField = 0;
  ValueLengthField = 0;
}

void DataElement::SetByteValue(const char * array, VL length)
{
  ByteValue * bv = new ByteValue(array,length);
  SetValue(*bv);
}

const ByteValue * DataElement::GetByteValue() const
{
  const ByteValue * bv = dynamic_cast<const ByteValue*>(ValueField.GetPointer());
  return bv;
}

SmartPointer<SequenceOfItems> DataElement::GetValueAsSQ() const
{
  if(IsEmpty()) return NULL;
  if(GetSequenceOfFragments()) return NULL;
  try
  {
    SequenceOfItems * sq =
      dynamic_cast<SequenceOfItems*>(ValueField.GetPointer());
    if (sq)
    {
      SmartPointer<SequenceOfItems> sqi = sq;
      return sqi;
    }
  }
  catch (std::bad_cast&) { ;; }
  const ByteValue * bv = GetByteValue();
  if(!bv) return NULL;
  SequenceOfItems * sq = new SequenceOfItems;
  sq->SetLength(bv->GetLength());
  if(GetVR() == VR::INVALID)
  {
    try
    {
      std::stringstream ss;
      ss.str(std::string(bv->GetPointer(), bv->GetLength()));
      sq->Read<ImplicitDataElement,SwapperNoOp>(ss, true);
      SmartPointer<SequenceOfItems> sqi = sq;
      return sqi;
    }
    catch(Exception &)
    {
      // fix a broken dicom implementation for Philips
      const Tag itemPMSStart(0xfeff, 0x00e0);
      // 0x3f3f, 0x3f00 ??
      std::stringstream ss;
      ss.str(std::string(bv->GetPointer(), bv->GetLength()));
      Tag item;
      item.Read<SwapperNoOp>(ss);
      if(item == itemPMSStart)
      {
        ss.seekg(-4,std::ios::cur);
        sq->Read<ExplicitDataElement,SwapperDoOp>(ss, true);
        SmartPointer<SequenceOfItems> sqi = sq;
        return sqi;
      }
    }
    catch(...) { ;; }
  }
  else if(GetVR() == VR::UN)
  {
    try
    {
      std::stringstream ss;
      ss.str(std::string(bv->GetPointer(), bv->GetLength()));
      sq->Read<ImplicitDataElement,SwapperNoOp>(ss, true);
      SmartPointer<SequenceOfItems> sqi = sq;
      return sqi;
    }
    catch(...) { ;; }
    try
    {
      std::stringstream ss;
      ss.str(std::string(bv->GetPointer(), bv->GetLength()));
      sq->Read<ExplicitDataElement,SwapperDoOp>(ss, true);
      SmartPointer<SequenceOfItems> sqi = sq;
      return sqi;
    }
    catch(...) { ;; }
    try
    {
      std::stringstream ss;
      ss.str(std::string(bv->GetPointer(), bv->GetLength()));
      sq->Read<ExplicitDataElement,SwapperNoOp>(ss, true);
      SmartPointer<SequenceOfItems> sqi = sq;
      return sqi;
    }
    catch(...) { ;; }
  }
  delete sq;
  return NULL;
}

const SequenceOfFragments * DataElement::GetSequenceOfFragments() const
{
  try
  {
    const SequenceOfFragments * sqf =
      dynamic_cast<SequenceOfFragments*>(ValueField.GetPointer());
    return sqf;
  }
  catch(std::bad_cast&) { ;; }
  return NULL;
}

SequenceOfFragments * DataElement::GetSequenceOfFragments()
{
  try
  {
    SequenceOfFragments * sqf =
      dynamic_cast<SequenceOfFragments*>(ValueField.GetPointer());
    return sqf;
  }
  catch(std::bad_cast&) { ;; }
  return NULL;
}

bool DataElement::IsUndefinedLength() const
{
  return ValueLengthField.IsUndefined();
}

void DataElement::SetValueFieldLength(VL vl, bool readvalues)
{
  if(readvalues) ValueField->SetLength(vl); // perform realloc
  else ValueField->SetLengthOnly(vl); // do not perform realloc
}

std::ostream & operator<<(std::ostream & os, const DataElement & val)
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

bool operator!=(const DataElement & lhs, const DataElement & rhs)
{
  return !(lhs == rhs);
}

} // end namespace mdcm
