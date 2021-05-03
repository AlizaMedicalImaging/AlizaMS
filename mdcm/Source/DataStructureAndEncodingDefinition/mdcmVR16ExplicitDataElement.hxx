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
#ifndef MDCMVR16EXPLICITDATAELEMENT_TXX
#define MDCMVR16EXPLICITDATAELEMENT_TXX

#include "mdcmSequenceOfItems.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmVL.h"
#include "mdcmParseException.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValueIO.h"
#include "mdcmSwapper.h"

namespace mdcm
{

template <typename TSwap>
std::istream &
VR16ExplicitDataElement::Read(std::istream & is)
{
  ReadPreValue<TSwap>(is);
  return ReadValue<TSwap>(is);
}

template <typename TSwap>
std::istream &
VR16ExplicitDataElement::ReadPreValue(std::istream & is)
{
  TagField.Read<TSwap>(is);
  // See PS 3.5, Data Element Structure With Explicit VR
  if (!is)
  {
    if (!is.eof())
    {
      assert(0 && "Should not happen");
    }
    return is;
  }
  assert(TagField != Tag(0xfffe, 0xe0dd));
  const Tag itemDelItem(0xfffe, 0xe00d);
  const Tag itemStartItem(0xfffe, 0x0000);
  assert(TagField != itemStartItem);
  if (TagField == itemDelItem)
  {
    if (!ValueLengthField.Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    if (ValueLengthField)
    {
      mdcmAlwaysWarnMacro("Item Delimitation Item has a length different from 0 and is: " << ValueLengthField);
    }
    ValueField = 0;
    return is;
  }

#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
  if (TagField == Tag(0x00ff, 0x4aa5))
  {
    assert(0 && "Should not happen");
  }
#endif
  // FIXME
  // Special hack for KONICA_VROX.dcm where in fact the VR=OX, in Pixel Data element
  // in which case we need to assume a 32bits VR...for now this is a big phat hack !
  bool OX_hack = false;
  try
  {
    if (!VRField.Read(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
  }
  catch (std::logic_error &)
  {
    VRField = VR::INVALID;
    // mdcm-MR-PHILIPS-16-Multi-Seq.dcm
    if (TagField == Tag(0xfffe, 0xe000))
    {
      mdcmAlwaysWarnMacro("Found item delimitor in item");
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    // -> For some reason VR is written as {44,0} well I guess this is a VR...
    // Technically there is a second bug, dcmtk assume other things when reading this tag,
    // so I need to change this tag too, if I ever want dcmtk to read this file. oh well
    // 0019004_Baseline_IMG1.dcm
    // -> VR is garbage also...
    if (TagField == Tag(0x7fe0, 0x0010))
    {
      OX_hack = true;
      VRField = VR::UN; // make it a fake 32bits for now
      char dummy[2];
      is.read(dummy, 2);
      assert(dummy[0] == 0 && dummy[1] == 0);
      mdcmAlwaysWarnMacro("Assuming 32 bits VR for Tag=" << TagField << " in order to read a buggy DICOM file.");
    }
    else
    {
      mdcmAlwaysWarnMacro("Assuming 16 bits VR for Tag=" << TagField << " in order to read a buggy DICOM file.");
    }
  }
  if (VR::GetLength(VRField) == 4)
  {
    if (!ValueLengthField.Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    if (OX_hack)
    {
      VRField = VR::INVALID;
    }
  }
  else
  {
    assert(OX_hack == false);
    // 16bits only
    if (!ValueLengthField.template Read16<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    // HACK for SIEMENS Leonardo
    if (ValueLengthField == 0x0006 && VRField == VR::UL && TagField.GetGroup() == 0x0009)
    {
      mdcmAlwaysWarnMacro("Replacing VL=0x0006 with VL=0x0004, for Tag=" << TagField
                                                                         << " in order to read a buggy DICOM file.");
      ValueLengthField = 0x0004;
    }
#endif
  }
  // I don't like the following 3 lines, what if 0000,0000 was indeed -wrongly- sent, we should be able to continue
  // chances is that 99% of times there is now way we can reach here, so safely throw an exception
  if (TagField == Tag(0x0000, 0x0000) && ValueLengthField == 0 && VRField == VR::INVALID)
  {
    // This handles DMCPACS_ExplicitImplicit_BogusIOP.dcm
    ParseException pe;
    pe.SetLastElement(*this);
    throw pe;
  }
  return is;
}

template <typename TSwap>
std::istream &
VR16ExplicitDataElement::ReadValue(std::istream & is, bool readvalues)
{
  if (is.eof())
    return is;
  if (ValueLengthField == 0)
  {
    ValueField = 0;
    return is;
  }
  if (VRField == VR::SQ)
  {
    // Check whether or not this is an undefined length sequence
    assert(TagField != Tag(0x7fe0, 0x0010));
    ValueField = new SequenceOfItems;
  }
  else if (ValueLengthField.IsUndefined())
  {
    if (VRField == VR::UN)
    {
      // Support cp246 conforming file:
      // Enhanced_MR_Image_Storage_PixelSpacingNotIn_0028_0030.dcm (illegal)
      // vs
      // undefined_length_un_vr.dcm
      assert(TagField != Tag(0x7fe0, 0x0010));
      ValueField = new SequenceOfItems;
      ValueField->SetLength(ValueLengthField);
      try
      {
        // cp246 compliant
        if (!ValueIO<ImplicitDataElement, TSwap>::Read(is, *ValueField, readvalues))
        {
          assert(0);
        }
      }
      catch (std::exception &)
      {
        // Must be one of those non-cp246 file...
        // but for some reason seekg back to previous offset + Read
        // as Explicit does not work
        ParseException pe;
        pe.SetLastElement(*this);
        throw pe;
      }
      return is;
    }
    else
    {
      if (TagField != Tag(0x7fe0, 0x0010))
      {
        // mdcmSampleData/ForSeriesTesting/Perfusion/DICOMDIR
        ParseException pe;
        pe.SetLastElement(*this);
        throw pe;
      }
      // Ok this is Pixel Data fragmented
      assert(TagField == Tag(0x7fe0, 0x0010));
      assert(VRField & VR::OB_OW);
      ValueField = new SequenceOfFragments;
    }
  }
  else
  {
    ValueField = new ByteValue;
  }
  // We have the length we should be able to read the value
  ValueField->SetLength(ValueLengthField); // perform realloc
#if defined(MDCM_SUPPORT_BROKEN_IMPLEMENTATION) && 0
  // PHILIPS_Intera-16-MONO2-Uncompress.dcm
  if (TagField == Tag(0x2001, 0xe05f) || TagField == Tag(0x2001, 0xe100) || TagField == Tag(0x2005, 0xe080) ||
      TagField == Tag(0x2005, 0xe083) || TagField == Tag(0x2005, 0xe084) || TagField == Tag(0x2005, 0xe402))
  // TagField.IsPrivate() && VRField == VR::SQ
  //-> Does not work for 0029
  // we really need to read item marker
  {
    mdcmWarningMacro("ByteSwaping Private SQ: " << TagField);
    assert(VRField == VR::SQ);
    assert(TagField.IsPrivate());
    try
    {
      if (!ValueIO<VR16ExplicitDataElement, SwapperDoOp>::Read(is, *ValueField, readvalues))
      {
        assert(0 && "Should not happen");
      }
      Value *           v = &*ValueField;
      SequenceOfItems * sq = dynamic_cast<SequenceOfItems *>(v);
      assert(sq);
      SequenceOfItems::Iterator it = sq->Begin();
      for (; it != sq->End(); ++it)
      {
        Item &         item = *it;
        DataSet &      ds = item.GetNestedDataSet();
        ByteSwapFilter bsf(ds);
        bsf.ByteSwap();
      }
    }
    catch (std::exception & ex)
    {
      ValueLengthField = ValueField->GetLength();
    }
    return is;
  }
#endif
  if (!ValueIO<VR16ExplicitDataElement, TSwap>::Read(is, *ValueField, readvalues))
  {
    // Might be the famous UN 16bits
    ParseException pe;
    pe.SetLastElement(*this);
    throw pe;
  }
  return is;
}

template <typename TSwap>
std::istream &
VR16ExplicitDataElement::ReadWithLength(std::istream & is, VL &)
{
  return Read<TSwap>(is);
}

} // end namespace mdcm

#endif // MDCMVR16EXPLICITDATAELEMENT_TXX
