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
#include "mdcmFileExplicitFilter.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmFragment.h"
#include "mdcmGlobal.h"
#include "mdcmDict.h"
#include "mdcmDicts.h"
#include "mdcmVR.h"
#include "mdcmVM.h"
#include "mdcmDataSetHelper.h"

/**
 * FileExplicitFilter class
 *
 * Changing an implicit dataset to an explicit dataset is NOT a
 * trivial task of simply changing the VR to the dict:
 *  - make sure SQ is properly set
 *  - recompute the explicit length SQ
 *  - make sure that VR is valid for the encoding
 *  - make sure that VR 16bits can store the original value length
 */

namespace mdcm
{

void
FileExplicitFilter::SetChangePrivateTags(bool b)
{
  ChangePrivateTags = b;
}

// When VR=16bits in explicit, but Implicit has a 32bits length, use VR=UN
void
FileExplicitFilter::SetUseVRUN(bool b)
{
  UseVRUN = b;
}

// By default set Sequence & Item length to Undefined to avoid recomputing length
void
FileExplicitFilter::SetRecomputeItemLength(bool b)
{
  RecomputeItemLength = b;
}

void
FileExplicitFilter::SetRecomputeSequenceLength(bool b)
{
  RecomputeSequenceLength = b;
}

void
FileExplicitFilter::SetFile(const File & f)
{
  F = f;
}

File &
FileExplicitFilter::GetFile()
{
  return *F;
}

bool
FileExplicitFilter::Change()
{
  const Global & g = GlobalInstance;
  const Dicts &  dicts = g.GetDicts();
  DataSet &      ds = F->GetDataSet();
  bool           b = ProcessDataSet(ds, dicts);
  return b;
}

bool
FileExplicitFilter::ProcessDataSet(DataSet & ds, Dicts const & dicts)
{
  if (RecomputeSequenceLength || RecomputeItemLength)
  {
    mdcmWarningMacro("Not implemented");
    return false;
  }
  DataSet::Iterator it = ds.Begin();
  for (; it != ds.End();)
  {
    DataElement  de = *it;
    std::string  strowner;
    const char * owner = NULL;
    const Tag &  t = de.GetTag();
    if (t.IsPrivate() &&
        !ChangePrivateTags
        // As a special exception we convert to proper VR:
        // - Private Group Length
        // - Private Creator
        // This makes the output more readable (and this should be relative safe)
        && !t.IsGroupLength() && !t.IsPrivateCreator())
    {
      ++it;
      continue;
    }
    if (t.IsPrivate() && !t.IsPrivateCreator())
    {
      strowner = ds.GetPrivateCreator(t);
      owner = strowner.c_str();
    }
    const DictEntry & entry = dicts.GetDictEntry(t, owner);
    const VR &        vr = entry.GetVR();
    VR                cvr = DataSetHelper::ComputeVR(*F, ds, t);
    VR                oldvr = de.GetVR();
    SmartPointer<SequenceOfItems> sqi = NULL;
    if (vr == VR::SQ || cvr == VR::SQ)
    {
      sqi = de.GetValueAsSQ();
      if (!sqi)
      {
        if (!de.IsEmpty())
        {
          mdcmWarningMacro("DICOM file may be unreadable");
          cvr = VR::UN;
        }
      }
    }
    if (de.GetByteValue() && !sqi)
    {
      if (cvr != VR::UN)
      {
        if (cvr & VR::VRASCII)
        {
          // mdcm-JPEG-Extended.dcm has a couple of VR::OB private field
          // is this a good idea to change them to an ASCII when we know this might not work?
          if (!(oldvr & VR::VRASCII || oldvr == VR::INVALID || oldvr == VR::UN))
          {
            mdcmErrorMacro("Cannot convert VR for tag: " << t << " " << oldvr << " is incompatible with " << cvr
                                                         << " as given by ref. dict.");
            return false;
          }
        }
        else if (cvr & VR::VRBINARY)
        {
          // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
          if (!(oldvr & VR::VRBINARY || oldvr == VR::INVALID || oldvr == VR::UN))
          {
            mdcmErrorMacro("Cannot convert VR for tag: " << t << " " << oldvr << " is incompatible with " << cvr
                                                         << " as given by ref. dict.");
            return false;
          }
        }
        else
        {
          assert(0); // programmer error
        }
        // one more check we are going to make this attribute explicit VR, there is
        // still a special case, when VL is > uint16_max then we must give up:
        if (!(cvr & VR::VL32) && de.GetVL() > UINT16_MAX)
        {
          cvr = VR::UN;
        }
        de.SetVR(cvr);
      }
    }
    else if (sqi)
    {
      assert(cvr == VR::SQ || cvr == VR::UN);
      de.SetVR(VR::SQ);
      if (de.GetByteValue())
      {
        de.SetValue(*sqi);
      }
      de.SetVLToUndefined();
      assert(sqi->GetLength().IsUndefined());
      // recursive
      SequenceOfItems::ItemVector::iterator sit = sqi->Items.begin();
      for (; sit != sqi->Items.end(); ++sit)
      {
        Item & item = *sit;
        item.SetVLToUndefined();
        DataSet & nds = item.GetNestedDataSet();
        ProcessDataSet(nds, dicts);
        item.SetVL(item.GetLength<ExplicitDataElement>());
      }
    }
    else if (de.GetSequenceOfFragments())
    {
      assert(cvr & VR::OB_OW);
    }
    else
    {
      // Ok length is 0, it can be a 0 length explicit SQ (implicit) or a ByteValue...
      // we cannot make any error here, simply change the VR
      de.SetVR(cvr);
    }
    ++it;
    ds.Replace(de);
  }
  return true;
}

} // end namespace mdcm
