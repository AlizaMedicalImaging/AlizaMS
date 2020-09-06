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
#include "mdcmImplicitDataElement.h"
#include "mdcmByteValue.h"
#include "mdcmSequenceOfItems.h"

namespace mdcm_ns
{

VL ImplicitDataElement::GetLength() const
{
  if(ValueLengthField.IsUndefined())
  {
    assert(ValueField->GetLength().IsUndefined());
    Value *p = ValueField;
    SequenceOfItems *sq = dynamic_cast<SequenceOfItems*>(p);
    if(sq)
    {
      return TagField.GetLength() + ValueLengthField.GetLength()
        + sq->ComputeLength<ImplicitDataElement>();
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    SequenceOfFragments *sf = dynamic_cast<SequenceOfFragments*>(p);
    if(sf)
    {
      return TagField.GetLength()
        + ValueLengthField.GetLength() + sf->ComputeLength();
    }
#endif
    assert(!ValueLengthField.IsUndefined());
    return ValueLengthField;
  }
  else if(const SequenceOfItems *sqi = dynamic_cast<const SequenceOfItems*>( ValueField.GetPointer()) )
  {
    return TagField.GetLength() + ValueLengthField.GetLength()
      + sqi->ComputeLength<ImplicitDataElement>();
  }
  else
  {
    assert(!ValueField || ValueField->GetLength() == ValueLengthField);
    return TagField.GetLength() + ValueLengthField.GetLength()
      + ValueLengthField;
  }
}

} // end namespace mdcm_ns
