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
#ifndef MDCMEXPLICITIMPLICITDATAELEMENT_TXX
#define MDCMEXPLICITIMPLICITDATAELEMENT_TXX

#include "mdcmExplicitImplicitDataElement.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmVL.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValueIO.h"
#include "mdcmSwapper.h"

namespace mdcm
{

template <typename TSwap>
std::istream &
ExplicitImplicitDataElement::Read(std::istream & is)
{
  ReadPreValue<TSwap>(is);
  return ReadValue<TSwap>(is);
}

template <typename TSwap>
std::istream &
ExplicitImplicitDataElement::ReadPreValue(std::istream & is)
{
  TagField.Read<TSwap>(is);
  // See PS 3.5, Data Element Structure With Explicit VR
  if (!is)
  {
    if (!is.eof()) // FIXME This should not be needed
    {
      assert(0 && "Should not happen");
    }
    return is;
  }
  if (TagField == Tag(0xfffe, 0xe0dd))
  {
    ParseException pe;
    pe.SetLastElement(*this);
    throw pe;
  }
  const Tag itemDelItem(0xfffe, 0xe00d);
  if (TagField == itemDelItem)
  {
    if (!ValueLengthField.Read<TSwap>(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    if (ValueLengthField)
    {
      mdcmDebugMacro("Item Delimitation Item has a length different from 0 and is: " << ValueLengthField);
    }
    ValueField = NULL;
    VRField = VR::INVALID;
    return is;
  }

#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
  if (TagField == Tag(0x00ff, 0x4aa5))
  {
    is.seekg(-4, std::ios::cur);
    TagField = Tag(0x7fe0, 0x0010);
    VRField = VR::OW;
    ValueField = new ByteValue;
    std::streampos s = is.tellg();
    is.seekg(0, std::ios::end);
    std::streampos e = is.tellg();
    is.seekg(s, std::ios::beg);
    ValueField->SetLength((int32_t)(e - s));
    ValueLengthField = ValueField->GetLength();
    const bool failed = !ValueIO<ExplicitDataElement, TSwap, uint16_t>::Read(is, *ValueField, true);
    if (failed)
    {
      throw std::logic_error("Exception");
    }
    return is;
  }
#endif
  try
  {
    if (!VRField.Read(is))
    {
      assert(0 && "Should not happen");
      return is;
    }
    if (VR::GetLength(VRField) == 4)
    {
      if (!ValueLengthField.Read<TSwap>(is))
      {
        assert(0 && "Should not happen");
        return is;
      }
    }
    else
    {
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
  }
  catch (const std::logic_error &)
  {
    VRField = VR::INVALID;
    is.seekg(-2, std::ios::cur);
    const Tag itemStartItem(0xfffe, 0xe000);
    if (TagField == itemStartItem)
      return is;
    if (!ValueLengthField.Read<TSwap>(is))
    {
      throw std::logic_error("Impossible");
    }
    if (ValueLengthField == 0)
    {
      ValueField = NULL;
      return is;
    }
    else if (ValueLengthField.IsUndefined())
    {
      // FIXME what if I am reading the pixel data
      if (TagField != Tag(0x7fe0, 0x0010))
      {
        ValueField = new SequenceOfItems;
      }
      else
      {
        mdcmAlwaysWarnMacro("Undefined value length is impossible in non-encapsulated Transfer Syntax");
        ValueField = new SequenceOfFragments;
      }
    }
    else
    {
      if (true)
      {
        ValueField = new ByteValue;
      }
      else
      {
        // In the following we read 4 more bytes in the Value field
        // to find out if this is a SQ or not
        // there is still work to do to handle the PMS featured SQ
        // where item Start is in fact 0xfeff, 0x00e0 ... sigh
        const Tag itemStart(0xfffe, 0xe000);
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
        const Tag itemPMSStart(0xfeff, 0x00e0);
        const Tag itemPMSStart2(0x3f3f, 0x3f00);
#endif
        Tag item;
        // TODO FIXME
        // This is pretty dumb to actually read to later on seekg back, why not `peek` directly?
        item.Read<TSwap>(is);
        // Maybe this code can later be rewritten as I believe that seek back
        // is very slow
        is.seekg(-4, std::ios::cur);
        if (item == itemStart)
        {
          assert(TagField != Tag(0x7fe0, 0x0010));
          ValueField = new SequenceOfItems;
        }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
        else if (item == itemPMSStart)
        {
          // MR_Philips_Intera_No_PrivateSequenceImplicitVR.dcm
          mdcmAlwaysWarnMacro("Illegal: Explicit SQ found in a file with "
                              "TransferSyntax=Implicit for tag: "
                              << TagField);
          // TODO: We READ Explicit ok, but we store Implicit !
          // Indeed when copying the VR will be saved, pretty cool eh?
          ValueField = new SequenceOfItems;
          ValueField->SetLength(ValueLengthField);
          try
          {
            if (!ValueIO<ExplicitDataElement, SwapperDoOp>::Read(is, *ValueField, true))
            {
              assert(0 && "Should not happen");
            }
          }
          catch (const std::exception & ex2)
          {
            (void)ex2;
            ValueLengthField = ValueField->GetLength();
          }
          return is;
        }
        else if (item == itemPMSStart2 && false)
        {
          mdcmAlwaysWarnMacro("Illegal: SQ start with " << itemPMSStart2 << " instead of " << itemStart
                                                        << " for tag: " << TagField);
          ValueField = new SequenceOfItems;
          ValueField->SetLength(ValueLengthField);
          if (!ValueIO<ImplicitDataElement, TSwap>::Read(is, *ValueField, true))
          {
            assert(0 && "Should not happen");
          }
          return is;
        }
#endif
        else
        {
          ValueField = new ByteValue;
        }
      }
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    // THE WORST BUG EVER. From GE Workstation
    if (ValueLengthField == 13)
    {
      // Historically GDCM did not enforce proper length
      // thus Theralys started writing illegal DICOM images
      const Tag theralys1(0x0008, 0x0070);
      const Tag theralys2(0x0008, 0x0080);
      if (TagField != theralys1 && TagField != theralys2)
      {
        mdcmAlwaysWarnMacro(
          "GE,13: Replacing VL=0x000d with VL=0x000a, for Tag=" << TagField << " in order to read a buggy DICOM file.");
        ValueLengthField = 10;
      }
    }
#endif
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    if (ValueLengthField == 0x31f031c && TagField == Tag(0x031e, 0x0324))
    {
      // TestImages/elbow.pap
      mdcmAlwaysWarnMacro("Replacing a VL. To be able to read a supposively broken Payrus file.");
      ValueLengthField = 202; // 0xca
    }
#endif
    // We have the length we should be able to read the value
    ValueField->SetLength(ValueLengthField);
    if (!ValueIO<ImplicitDataElement, TSwap>::Read(is, *ValueField, true))
    {
      // Special handling for PixelData tag
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
      if (TagField == Tag(0x7fe0, 0x0010))
      {
        mdcmAlwaysWarnMacro("Pixel Data may be corrupted");
        is.clear();
      }
      else
#endif
      {
        throw std::logic_error("Should not happen (imp)");
      }
      return is;
    }
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    // dcmtk 3.5.4 is resilient to broken explicit SQ length and will properly recompute it
    // as long as each of the Item lengths are correct
    VL dummy = ValueField->GetLength();
    if (ValueLengthField != dummy)
    {
      mdcmAlwaysWarnMacro("ValueLengthField was bogus");
      assert(0);
      ValueLengthField = dummy;
    }
#else
    assert(ValueLengthField == ValueField->GetLength());
    assert(VRField == VR::INVALID);
#endif
    return is;
  }
  // I don't like the following 3 lines, what if 0000,0000 was indeed -wrongly- sent, we should be able to continue
  // chances is that 99% of times there is now way we can reach here, so safely throw an exception
  if (TagField == Tag(0x0000, 0x0000) && ValueLengthField == 0 && VRField == VR::INVALID)
  {
    ParseException pe;
    pe.SetLastElement(*this);
    throw pe;
  }
#ifdef ELSCINT1_01F7_1070
  if (TagField == Tag(0x01f7, 0x1070))
  {
    ValueLengthField = ValueLengthField - 7;
  }
#endif
  return is;
}

template <typename TSwap>
std::istream &
ExplicitImplicitDataElement::ReadValue(std::istream & is, bool readvalues)
{
  if (is.eof())
    return is;
  /* thechnically the following is bad
     it assumes that in the case of explicit/implicit dataset
     we are not handle the prevalue call properly for buggy implicit attribute
   */
  if (VRField == VR::INVALID)
    return is;
  if (ValueLengthField == 0)
  {
    ValueField = NULL;
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
      catch (const std::exception &)
      {
        // Must be one of those non-cp246 file,
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
  this->SetValueFieldLength(ValueLengthField, readvalues);
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
      if (!ValueIO<ExplicitDataElement, SwapperDoOp>::Read(is, *ValueField, readvalues))
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
    catch (const std::exception & ex)
    {
      ValueLengthField = ValueField->GetLength();
    }
    return is;
  }
#endif
  bool failed;
  if (VRField & VR::VRASCII)
  {
    failed = !ValueIO<ExplicitDataElement, TSwap>::Read(is, *ValueField, readvalues);
  }
  else
  {
    assert(VRField & VR::VRBINARY);
    unsigned int vrsize = VRField.GetSize();
    assert(vrsize == 1 || vrsize == 2 || vrsize == 4 || vrsize == 8);
    if (VRField == VR::AT)
      vrsize = 2;
    switch (vrsize)
    {
      case 1:
        failed = !ValueIO<ExplicitImplicitDataElement, TSwap, uint8_t>::Read(is, *ValueField, readvalues);
        break;
      case 2:
        failed = !ValueIO<ExplicitImplicitDataElement, TSwap, uint16_t>::Read(is, *ValueField, readvalues);
        break;
      case 4:
        failed = !ValueIO<ExplicitImplicitDataElement, TSwap, uint32_t>::Read(is, *ValueField, readvalues);
        break;
      case 8:
        failed = !ValueIO<ExplicitImplicitDataElement, TSwap, uint64_t>::Read(is, *ValueField, readvalues);
        break;
      default:
        failed = true;
        assert(0);
    }
  }
  if (failed)
  {
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    if (TagField == Tag(0x7fe0, 0x0010))
    {
      // BUG this should be moved to the ImageReader class, only this class knows
      // what 7fe0 actually is, and should tolerate partial Pixel Data element...
      // PMS-IncompletePixelData.dcm
      mdcmAlwaysWarnMacro("Incomplete Pixel Data found, use file at own risk");
      is.clear();
    }
    else
#endif
    {
      // Might be the famous UN 16bits
      ParseException pe;
      pe.SetLastElement(*this);
      throw pe;
    }
    return is;
  }
  return is;
}

} // end namespace mdcm

#endif // MDCMEXPLICITIMPLICITDATAELEMENT_TXX
