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
  void DataElement::SetVLToUndefined()
  {
    try
    {
      SequenceOfItems * sqi =
        dynamic_cast<SequenceOfItems*>(ValueField.GetPointer());
      if(sqi) sqi->SetLengthToUndefined();
    }
    catch (std::bad_cast&) { ;; }
    ValueLengthField.SetToUndefined();
  }

  const SequenceOfFragments * DataElement::GetSequenceOfFragments() const
  {
    try
    {
      const SequenceOfFragments * sqf =
        dynamic_cast<SequenceOfFragments*>(ValueField.GetPointer());
      return sqf;
    }
    catch (std::bad_cast&) { ;; }
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
    catch (std::bad_cast&) { ;; }
    return NULL;
  }

  SmartPointer<SequenceOfItems> DataElement::GetValueAsSQ() const
  {
    if (IsEmpty()) return NULL;
    if (GetSequenceOfFragments()) return NULL;

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
    if (!bv) return NULL;
    SequenceOfItems * sq = new SequenceOfItems;
    sq->SetLength(bv->GetLength());
    if (GetVR() == VR::INVALID)
    {
      try
      {
        std::stringstream ss;
        ss.str(std::string(bv->GetPointer(), bv->GetLength()));
        sq->Read<ImplicitDataElement,SwapperNoOp>(ss, true);
        SmartPointer<SequenceOfItems> sqi = sq;
        return sqi;
      }
      catch (Exception &)
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
      catch (...) { ;; }
    }
    else if (GetVR() == VR::UN)
    {
      try
      {
        std::stringstream ss;
        ss.str(std::string(bv->GetPointer(), bv->GetLength()));
        sq->Read<ImplicitDataElement,SwapperNoOp>(ss, true);
        SmartPointer<SequenceOfItems> sqi = sq;
        return sqi;
      }
      catch (...) { ;; }
      try
      {
        std::stringstream ss;
        ss.str(std::string(bv->GetPointer(), bv->GetLength()));
        sq->Read<ExplicitDataElement,SwapperDoOp>(ss, true);
        SmartPointer<SequenceOfItems> sqi = sq;
        return sqi;
      }
      catch (...) { ;; }
      try
      {
        std::stringstream ss;
        ss.str(std::string(bv->GetPointer(), bv->GetLength()));
        sq->Read<ExplicitDataElement,SwapperNoOp>(ss, true);
        SmartPointer<SequenceOfItems> sqi = sq;
        return sqi;
      }
      catch (...) { ;; }
    }
    delete sq;
    return NULL;
  }

void DataElement::SetValueFieldLength(VL vl, bool readvalues)
{
  if (readvalues) ValueField->SetLength(vl); // perform realloc
  else ValueField->SetLengthOnly(vl); // do not perform realloc
}

} // end namespace mdcm
