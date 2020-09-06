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

#include "mdcmByteSwapFilter.h"
#include "mdcmElement.h"
#include "mdcmByteValue.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmSwapper.h"

namespace mdcm
{

ByteSwapFilter::~ByteSwapFilter()
{
}

bool ByteSwapFilter::ByteSwap()
{
  for(DataSet::ConstIterator it = DS.Begin(); it != DS.End(); ++it)
  {
    const DataElement & de = *it;
    VR const & vr = de.GetVR();
    ByteValue * bv = const_cast<ByteValue*>(de.GetByteValue());
    mdcm::SmartPointer<mdcm::SequenceOfItems> si = de.GetValueAsSQ();
    if(de.IsEmpty())
    {
      ;;
    }
    else if(bv && !si)
    {
      assert(!si);
      if(vr & VR::VRBINARY)
      {
        switch(vr)
        {
        case VR::OB:
          break;
        case VR::SS:
          SwapperDoOp::SwapArray(
            (int16_t*)bv->GetPointer(), bv->GetLength()/sizeof(int16_t));
          break;
        case VR::OW:
        case VR::US:
          SwapperDoOp::SwapArray(
            (uint16_t*)bv->GetPointer(), bv->GetLength()/sizeof(uint16_t));
          break;
        case VR::SL:
          SwapperDoOp::SwapArray(
            (int32_t*)bv->GetPointer(), bv->GetLength()/sizeof(int32_t));
          break;
        case VR::OL:
        case VR::UL:
          SwapperDoOp::SwapArray(
            (uint32_t*)bv->GetPointer(), bv->GetLength()/sizeof(uint32_t));
          break;
        case VR::OV:
        case VR::UV:
          SwapperDoOp::SwapArray(
            (uint64_t*)bv->GetPointer(), bv->GetLength()/sizeof(uint64_t));
          break;
        case VR::SV:
          SwapperDoOp::SwapArray(
            (int64_t*)bv->GetPointer(), bv->GetLength()/sizeof(int64_t));
          break;
        case VR::FL:
        case VR::OF:
        case VR::FD:
        case VR::OD:
          break;
        default:
          assert(0 && "Should not happen");
          break;
        }
      }
    }
    else if(si)
    {
      if(si && si->GetNumberOfItems() > 0)
      {
        SequenceOfItems::ConstIterator it2 = si->Begin();
        for(; it2 != si->End(); ++it2)
        {
          const Item & item = *it2;
          DataSet & ds = const_cast<DataSet&>(item.GetNestedDataSet());
          ByteSwapFilter bsf(ds);
          bsf.ByteSwap();
        }
      }
    }
    else if(const SequenceOfFragments * sf = de.GetSequenceOfFragments())
    {
      (void)sf;
      assert(0 && "Should not happen");
    }
    else
    {
      assert(0 && "error");
    }
  }
  if(ByteSwapTag)
  {
    DataSet copy;
    DataSet::ConstIterator it = DS.Begin();
    for(; it != DS.End(); ++it)
    {
      DataElement de = *it;
      const Tag & tag = de.GetTag();
      de.SetTag(
        Tag(
          SwapperDoOp::Swap(
            tag.GetGroup()),
          SwapperDoOp::Swap(
            tag.GetElement())));
      copy.Insert(de);
      DS.Remove(de.GetTag());
    }
    DS = copy;
  }
  return true;
}

}
