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

#include "mdcmPixmapReader.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValue.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmElement.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmTransferSyntax.h"
#include "mdcmLookupTable.h"
#include "mdcmAttribute.h"
#include "mdcmIconImage.h"
#include "mdcmPrivateTag.h"
#include "mdcmJPEGCodec.h"
#include "mdcmImageHelper.h"
#include <utility>

namespace
{

void
DoIconImage(const mdcm::DataSet & rootds, mdcm::Pixmap & image)
{
  const mdcm::Tag   ticonimage(0x0088, 0x0200);
  mdcm::IconImage & pixeldata = image.GetIconImage();
  const mdcm::DataElement & iconimagesq = rootds.GetDataElement(ticonimage);
  if (!iconimagesq.IsEmpty())
  {
    mdcm::SmartPointer<mdcm::SequenceOfItems> sq = iconimagesq.GetValueAsSQ();
    if (!sq || sq->IsEmpty())
      return;
    mdcm::SequenceOfItems::ConstIterator it = sq->Begin();
    const mdcm::DataSet &                ds = it->GetNestedDataSet();
    {
      mdcm::Attribute<0x0028, 0x0011> at = { 0 };
      at.SetFromDataSet(ds);
      pixeldata.SetDimension(0, at.GetValue());
    }
    {
      mdcm::Attribute<0x0028, 0x0010> at = { 0 };
      at.SetFromDataSet(ds);
      pixeldata.SetDimension(1, at.GetValue());
    }
    mdcm::PixelFormat pf;
    {
      mdcm::Attribute<0x0028, 0x0100> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsAllocated(at.GetValue());
    }
    {
      mdcm::Attribute<0x0028, 0x0101> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsStored(at.GetValue());
    }
    {
      mdcm::Attribute<0x0028, 0x0102> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetHighBit(at.GetValue());
    }
    {
      mdcm::Attribute<0x0028, 0x0103> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetPixelRepresentation(at.GetValue());
    }
    {
      mdcm::Attribute<0x0028, 0x0002> at = { 1 };
      at.SetFromDataSet(ds);
      pf.SetSamplesPerPixel(at.GetValue());
    }
    pixeldata.SetPixelFormat(pf);
    mdcm::PhotometricInterpretation pi = mdcm::PhotometricInterpretation::MONOCHROME2;
    const mdcm::DataElement & photometricinterpretation_de = ds.GetDataElement(mdcm::Tag(0x0028, 0x0004));
    if (!photometricinterpretation_de.IsEmpty())
    {
      const mdcm::ByteValue * photometricinterpretation = photometricinterpretation_de.GetByteValue();
      std::string             photometricinterpretation_str(
        photometricinterpretation->GetPointer(),
        photometricinterpretation->GetLength());
      pi = mdcm::PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str());
    }
    assert(pi != mdcm::PhotometricInterpretation::UNKNOWN);
    pixeldata.SetPhotometricInterpretation(pi);
    if (pi == mdcm::PhotometricInterpretation::PALETTE_COLOR)
    {
      mdcm::LookupTable lut;
      lut.Allocate(pf.GetBitsAllocated());
      for (int i = 0; i < 3; ++i)
      {
        const mdcm::Tag                            tdescriptor(0x0028, static_cast<uint16_t>(0x1101 + i));
        mdcm::Element<mdcm::VR::US, mdcm::VM::VM3> el_us3;
        el_us3.SetFromDataElement(ds[tdescriptor]);
        lut.InitializeLUT(mdcm::LookupTable::LookupTableType(i), el_us3[0], el_us3[1], el_us3[2]);
        const mdcm::DataElement & lut_de = ds.GetDataElement(mdcm::Tag(0x0028, static_cast<uint16_t>(0x1201 + i)));
        if (!lut_de.IsEmpty())
        {
          const mdcm::ByteValue * lut_raw = lut_de.GetByteValue();
          if (lut_raw)
          {
            lut.SetLUT(
              mdcm::LookupTable::LookupTableType(i),
              reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
              lut_raw->GetLength());
            unsigned int check = (el_us3.GetValue(0) ? el_us3.GetValue(0) : 65536) * el_us3.GetValue(2) / 8;
            assert(check == lut_raw->GetLength() || 2 * check == lut_raw->GetLength() || check + 1 == lut_raw->GetLength());
            (void)check;
          }
          else
          {
            mdcmWarningMacro("Icon Sequence is incomplete (1)");
            pixeldata.ClearDimensions();
            return;
          }
        }
        else
        {
          const mdcm::DataElement & seglut_de = ds.GetDataElement(mdcm::Tag(0x0028, static_cast<uint16_t>(0x1221 + i)));
          if (!seglut_de.IsEmpty())
          {
            const mdcm::ByteValue * lut_raw = seglut_de.GetByteValue();
            if (lut_raw)
            {
              lut.SetSegmentedLUT(
                mdcm::LookupTable::LookupTableType(i),
                reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
                lut_raw->GetLength());
            }
            else
            {
              mdcmWarningMacro("Icon Sequence is incomplete (2)");
              pixeldata.ClearDimensions();
              return;
            }
          }
          else
          {
            mdcmWarningMacro("Icon Sequence is incomplete (3)");
            pixeldata.ClearDimensions();
            return;
          }
        }
      }
      pixeldata.SetLUT(std::move(lut));
    }
    const mdcm::DataElement & de = ds.GetDataElement(mdcm::Tag(0x7fe0, 0x0010));
    if (de.IsEmpty())
    {
      mdcmWarningMacro("Icon Sequence is incomplete (4)");
      pixeldata.ClearDimensions();
      return;
    }
    pixeldata.SetDataElement(de);
    // Pass TransferSyntax:
    // Warning This is legal for the Icon to be uncompress in a compressed image
    // We need to set the appropriate TS here
    const mdcm::ByteValue * bv = de.GetByteValue();
    if (bv)
      pixeldata.SetTransferSyntax(mdcm::TransferSyntax::ImplicitVRLittleEndian);
    else
      pixeldata.SetTransferSyntax(image.GetTransferSyntax());
  }
}

void
DoCurves(const mdcm::DataSet & ds, mdcm::Pixmap & pixeldata)
{
  const unsigned int numcurves = mdcm::Curve::GetNumberOfCurves(ds);
  if (numcurves > 0)
  {
    pixeldata.SetNumberOfCurves(numcurves);
    mdcm::Tag    curve(0x5000, 0x0000);
    bool         finished = false;
    unsigned int idxcurves = 0;
    while (!finished)
    {
      const mdcm::DataElement & de = ds.FindNextDataElement(curve);
      if (de.GetTag().GetGroup() > 0x50FF)
      {
        finished = true;
      }
      else if (de.GetTag().IsPrivate()) // GEMS owns some 0x5003
      {
        curve.SetGroup(static_cast<uint16_t>(de.GetTag().GetGroup() + 1));
        curve.SetElement(0);
      }
      else
      {
        mdcm::Curve & ov = pixeldata.GetCurve(idxcurves);
        ++idxcurves;
        curve = de.GetTag();
        uint16_t currentcurve = curve.GetGroup();
        assert(!(currentcurve % 2));
        mdcm::DataElement de2 = de;
        while (de2.GetTag().GetGroup() == currentcurve)
        {
          ov.Update(de2);
          curve.SetElement(static_cast<uint16_t>(de2.GetTag().GetElement() + 1));
          de2 = ds.FindNextDataElement(curve);
        }
      }
    }
    assert(idxcurves == numcurves);
  }
}

unsigned int
GetNumberOfOverlaysInternal(const mdcm::DataSet & ds, std::vector<uint16_t> & overlaylist)
{
  mdcm::Tag    overlay(0x6000, 0x0000);
  bool         finished = false;
  unsigned int numoverlays = 0;
  while (!finished)
  {
    const mdcm::DataElement & de = ds.FindNextDataElement(overlay);
    if (de.GetTag().GetGroup() > 0x60FF)
    {
      finished = true;
    }
    else if (de.GetTag().IsPrivate())
    {
      overlay.SetGroup(static_cast<uint16_t>(de.GetTag().GetGroup() + 1));
      overlay.SetElement(0);
    }
    else
    {
      // Store found tag in overlay
      overlay = de.GetTag();
      mdcm::Tag toverlaydata(overlay.GetGroup(), 0x3000);
      mdcm::Tag toverlayrows(overlay.GetGroup(), 0x0010);
      mdcm::Tag toverlaycols(overlay.GetGroup(), 0x0011);
      mdcm::Tag toverlaybitpos(overlay.GetGroup(), 0x0102);
      const mdcm::DataElement & overlaydata = ds.GetDataElement(toverlaydata);
      if (!overlaydata.IsEmpty())
      {
        ++numoverlays;
        overlaylist.push_back(overlay.GetGroup());
      }
      else
      {
        const mdcm::DataElement & overlayrows = ds.GetDataElement(toverlayrows);
        const mdcm::DataElement & overlaycols = ds.GetDataElement(toverlaycols);
        const mdcm::DataElement & overlaybitpos = ds.GetDataElement(toverlaybitpos);
        if (!overlayrows.IsEmpty() && !overlaycols.IsEmpty() && !overlaybitpos.IsEmpty())
        {
          ++numoverlays;
          overlaylist.push_back(overlay.GetGroup());
        }
      }
      overlay.SetGroup(static_cast<uint16_t>(overlay.GetGroup() + 2));
      overlay.SetElement(0);
    }
  }
  assert(numoverlays < 0x00ff / 2);
  // PS 3.3 - 2004:
  // C.9.2 Overlay plane module
  // Each Overlay Plane is one bit deep. Sixteen separate Overlay Planes may be associated with an
  // Image or exist as Standalone Overlays in a Series
  assert(numoverlays <= 16);
  assert(numoverlays == overlaylist.size());
  return numoverlays;
}

bool
DoOverlays(const mdcm::DataSet & ds, mdcm::Pixmap & pixeldata)
{
  std::vector<uint16_t> overlaylist;
  std::vector<bool>     updateoverlayinfo;
  const unsigned int    numoverlays = GetNumberOfOverlaysInternal(ds, overlaylist);
  if (numoverlays > 0)
  {
    updateoverlayinfo.resize(numoverlays, false);
    pixeldata.SetNumberOfOverlays(numoverlays);
    for (unsigned int idxoverlays = 0; idxoverlays < numoverlays; ++idxoverlays)
    {
      mdcm::Overlay & ov = pixeldata.GetOverlay(idxoverlays);
      uint16_t  currentoverlay = overlaylist[idxoverlays];
      mdcm::Tag overlay(0x6000, 0x0000);
      overlay.SetGroup(currentoverlay);
      const mdcm::DataElement & de = ds.FindNextDataElement(overlay);
      assert(!(currentoverlay % 2));
      mdcm::DataElement de2 = de;
      while (de2.GetTag().GetGroup() == currentoverlay)
      {
        ov.Update(de2);
        overlay.SetElement(static_cast<uint16_t>(de2.GetTag().GetElement() + 1));
        de2 = ds.FindNextDataElement(overlay);
      }
      std::ostringstream unpack;
      ov.Decompress(unpack);
      if (!ov.IsEmpty())
      {
        assert(ov.IsInPixelData() == false);
      }
      else if (pixeldata.GetPixelFormat().GetSamplesPerPixel() == 1)
      {
        ov.IsInPixelData(true);
        if (ov.GetBitsAllocated() != pixeldata.GetPixelFormat().GetBitsAllocated())
        {
          mdcmWarningMacro("Bits Allocated are wrong. Correcting.");
          ov.SetBitsAllocated(pixeldata.GetPixelFormat().GetBitsAllocated());
        }
        if (!ov.GrabOverlayFromPixelData(ds))
        {
          mdcmWarningMacro("Could not extract overlay from pixel data (1)");
        }
        updateoverlayinfo[idxoverlays] = true;
      }
      else
      {
        mdcmWarningMacro("Could not extract overlay from pixel data (2)");
      }
    }
  }
  const mdcm::PixelFormat & pf = pixeldata.GetPixelFormat();
  for (size_t ov_idx = pixeldata.GetNumberOfOverlays(); ov_idx != 0; --ov_idx)
  {
    size_t                ov = ov_idx - 1;
    const mdcm::Overlay & o = pixeldata.GetOverlay(ov);
    if (o.IsInPixelData())
    {
      unsigned short obp = o.GetBitPosition();
      if (obp < pf.GetBitsStored())
      {
        pixeldata.RemoveOverlay(ov);
        updateoverlayinfo.erase(updateoverlayinfo.begin() + ov);
        mdcmWarningMacro("Invalid: " << obp << " < GetBitsStored()");
      }
      else if (obp > pf.GetBitsAllocated())
      {
        pixeldata.RemoveOverlay(ov);
        updateoverlayinfo.erase(updateoverlayinfo.begin() + ov);
        mdcmWarningMacro("Invalid: " << obp << " > GetBitsAllocated()");
      }
    }
  }
  for (size_t ov = 0; ov < pixeldata.GetNumberOfOverlays() && updateoverlayinfo[ov]; ++ov)
  {
    mdcm::Overlay & o = pixeldata.GetOverlay(ov);
    if (o.GetBitsAllocated() == 16)
    {
      o.SetBitsAllocated(1);
      o.SetBitPosition(0);
    }
    else
    {
      mdcmErrorMacro("Overlay #" << ov << " is not supported");
      return false;
    }
  }
  return true;
}

}

namespace mdcm
{

PixmapReader::PixmapReader() : PixelData(new Pixmap)
{}

void
PixmapReader::SetApplySupplementalLUT(bool t)
{
  m_AlppySupplementalLUT = t;
}

bool
PixmapReader::GetApplySupplementalLUT() const
{
  return m_AlppySupplementalLUT;
}

void
PixmapReader::SetProcessOverlays(bool t)
{
  m_ProcessOverlays = t;
}

bool
PixmapReader::GetProcessOverlays() const
{
  return m_ProcessOverlays;
}

void
PixmapReader::SetProcessIcons(bool t)
{
  m_ProcessIcons = t;
}

bool
PixmapReader::GetProcessIcons() const
{
  return m_ProcessIcons;
}

void
PixmapReader::SetProcessCurves(bool t)
{
  m_ProcessCurves = t;
}

bool
PixmapReader::GetProcessCurves() const
{
  return m_ProcessCurves;
}

// Valid only after a call to Read
const Pixmap &
PixmapReader::GetPixmap() const
{
  return *PixelData;
}

// Valid only after a call to Read
Pixmap &
PixmapReader::GetPixmap()
{
  return *PixelData;
}

bool
PixmapReader::Read()
{
  if (!Reader::Read())
  {
    return false;
  }
  const FileMetaInformation & header = F->GetHeader();
  const DataSet &             ds = F->GetDataSet();
  const TransferSyntax &      ts = header.GetDataSetTransferSyntax();
  PixelData->SetTransferSyntax(ts);
  bool         res = false;
  MediaStorage ms = header.GetMediaStorage();
  bool         isImage = MediaStorage::IsImage(ms);
  if (isImage)
  {
    assert(ts != TransferSyntax::TS_END && ms != MediaStorage::MS_END);
    res = ReadImage(ms);
  }
  else
  {
    MediaStorage ms2 = ds.GetMediaStorage();
    if (ms == MediaStorage::MediaStorageDirectoryStorage && ms2 == MediaStorage::MS_END)
    {
      mdcmDebugMacro("DICOM file is not an image but " << MediaStorage::GetMSString(ms) << " SOP Class UID");
      res = false;
    }
    else if (ms == ms2 && ms != MediaStorage::MS_END)
    {
      mdcmDebugMacro("DICOM file is not an image but " << MediaStorage::GetMSString(ms) << " SOP Class UID");
      res = false;
    }
    else
    {
      if (ms2 != MediaStorage::MS_END)
      {
        bool isImage2 = MediaStorage::IsImage(ms2);
        if (isImage2)
        {
          mdcmDebugMacro("It might be a DICOM file, e.g. Mallinckrodt-like");
          res = ReadImage(ms2);
        }
        else
        {
          ms2.SetFromFile(*F);
          if (MediaStorage::IsImage(ms2))
          {
            res = ReadImage(ms2);
          }
          else
          {
            mdcmDebugMacro("DICOM file is not an image but " << MediaStorage::GetMSString(ms2)
                           << " SOP Class UID");
            res = false;
          }
        }
      }
      else if (ts == TransferSyntax::ImplicitVRBigEndianACRNEMA || header.IsEmpty())
      {
        mdcmDebugMacro("Looks like an ACR-NEMA file");
        res = ReadACRNEMAImage();
      }
      else
      {
        assert(ts != TransferSyntax::TS_END && ms == MediaStorage::MS_END);
        mdcmWarningMacro("Trying to read this file as a DICOM file");
        MediaStorage ms3;
        ms3.SetFromFile(GetFile());
        if (ms3 != MediaStorage::MS_END)
        {
          res = ReadImage(ms3);
        }
        else
        {
#if 1
          mdcmAlwaysWarnMacro("Trying to read unknown Media Storage as image"); 
          res = ReadImage(ms3);
#else
          res = false;
#endif
        }
      }
    }
  }
  return res;
}

bool
PixmapReader::ReadImageInternal(const MediaStorage & ms, bool handlepixeldata)
{
  const DataSet &     ds = F->GetDataSet();
  bool                isacrnema{};
  const Tag           trecognitioncode(0x0008, 0x0010);
  const Tag           pixeldataf = Tag(0x7fe0, 0x0008);
  const Tag           pixeldatad = Tag(0x7fe0, 0x0009);
  const Tag           pixeldata = Tag(0x7fe0, 0x0010);
  const DataElement & recognitioncode_de = ds.GetDataElement(trecognitioncode);
  if (!recognitioncode_de.IsEmpty())
  {
    mdcmDebugMacro("Mixture of ACR NEMA and DICOM file");
    isacrnema = true;
    const char * str = recognitioncode_de.GetByteValue()->GetPointer();
    assert(strncmp(str, "ACR-NEMA", strlen("ACR-NEMA")) == 0 || strncmp(str, "ACRNEMA", strlen("ACRNEMA")) == 0);
    (void)str;
  }
  std::vector<unsigned int> vdims = ImageHelper::GetDimensionsValue(*F);
  unsigned int              numberofframes = vdims[2];
  if (numberofframes > 1)
  {
    PixelData->SetNumberOfDimensions(3);
    PixelData->SetDimension(2, numberofframes);
  }
  else
  {
    mdcmDebugMacro("NumberOfFrames was specified with a value of: " << numberofframes);
    PixelData->SetNumberOfDimensions(2);
  }
  PixelData->SetDimension(0, vdims[0]);
  PixelData->SetDimension(1, vdims[1]);
  PixelFormat pf;
  {
    Attribute<0x0028, 0x0002> at = { 1 }; // assume 1 sample per pixel
    at.SetFromDataSet(ds);
    pf.SetSamplesPerPixel(at.GetValue());
  }
  const bool pixeldata_ = ds.FindDataElement(pixeldata);
  const bool pixeldatad_ = ds.FindDataElement(pixeldatad);
  const bool pixeldataf_ = ds.FindDataElement(pixeldataf);
  if (ms == MediaStorage::ParametricMapStorage)
  {
    Attribute<0x0028, 0x0100> at = { 0 };
    at.SetFromDataSet(ds);
    const unsigned short bit_allocated = at.GetValue();
    if (bit_allocated == 32)
    {
      pf.SetBitsAllocated(32);
      pf.SetBitsStored(32);
      pf.SetHighBit(31);
      pf.SetScalarType(PixelFormat::FLOAT32);
    }
    else if (bit_allocated == 64 && pixeldatad_)
    {
      pf.SetBitsAllocated(64);
      pf.SetBitsStored(64);
      pf.SetHighBit(63);
      pf.SetScalarType(PixelFormat::FLOAT64);
    }
    else
    {
      return false;
    }
  }
#if 1
  else if (!pixeldata_ && (pixeldataf_ || pixeldatad_))
  {
    Attribute<0x0028, 0x0100> at = { 0 };
    at.SetFromDataSet(ds);
    const unsigned short bit_allocated = at.GetValue();
    if (bit_allocated == 32)
    {
      pf.SetBitsAllocated(32);
      pf.SetBitsStored(32);
      pf.SetHighBit(31);
      pf.SetScalarType(PixelFormat::FLOAT32);
    }
    else if (bit_allocated == 64 && pixeldatad_)
    {
      pf.SetBitsAllocated(64);
      pf.SetBitsStored(64);
      pf.SetHighBit(63);
      pf.SetScalarType(PixelFormat::FLOAT64);
    }
    else
    {
      return false;
    }
  }
#endif
  else
  {
    assert(MediaStorage::IsImage(ms));
    {
      Attribute<0x0028, 0x0100> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsAllocated(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0101> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsStored(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0102> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetHighBit(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0103> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetPixelRepresentation(at.GetValue());
    }
  }
  // Photometric Interpretation
  const Tag         tphotometricinterpretation(0x0028, 0x0004);
  const ByteValue * photometricinterpretation = ImageHelper::GetPointerFromElement(tphotometricinterpretation, *F);
  PhotometricInterpretation pi = PhotometricInterpretation::UNKNOWN;
  if (photometricinterpretation)
  {
    std::string photometricinterpretation_str(photometricinterpretation->GetPointer(),
                                              photometricinterpretation->GetLength());
    pi = PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str());
    if (pi == PhotometricInterpretation::PI_END)
    {
      mdcmWarningMacro("Discarding suspicious PhotometricInterpretation found: " << photometricinterpretation_str);
    }
  }
  if (!photometricinterpretation || pi == PhotometricInterpretation::PI_END)
  {
    if (pf.GetSamplesPerPixel() == 1)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to MONOCHROME2");
      pi = PhotometricInterpretation::MONOCHROME2;
    }
    else if (pf.GetSamplesPerPixel() == 3)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to RGB");
      pi = PhotometricInterpretation::RGB;
    }
    else if (pf.GetSamplesPerPixel() == 4)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to ARGB");
      pi = PhotometricInterpretation::ARGB;
    }
    else
    {
      mdcmWarningMacro("Impossible value for Samples Per Pixel: " << pf.GetSamplesPerPixel());
      return false;
    }
  }
  assert(pi != PhotometricInterpretation::PI_END);
  if (!pf.GetSamplesPerPixel() || (pi.GetSamplesPerPixel() != pf.GetSamplesPerPixel()))
  {
    if (pi != PhotometricInterpretation::UNKNOWN)
    {
      pf.SetSamplesPerPixel(pi.GetSamplesPerPixel());
    }
    else if (isacrnema)
    {
      assert(pf.GetSamplesPerPixel() == 0);
      assert(pi == PhotometricInterpretation::UNKNOWN);
      pf.SetSamplesPerPixel(1);
      pi = PhotometricInterpretation::MONOCHROME2;
    }
    else
    {
      mdcmAlwaysWarnMacro("Cannot recognize image type. Does not looks like ACR-NEMA and "
                          "is missing both Sample Per Pixel AND PhotometricInterpretation.");
      return false;
    }
  }
  assert(pf.GetSamplesPerPixel() != 0);
  // Very important to set the PixelFormat here before PlanarConfiguration
  PixelData->SetPixelFormat(pf);
  pf = PixelData->GetPixelFormat();
  if (!pf.IsValid())
  {
    return false;
  }
  if (pi == PhotometricInterpretation::UNKNOWN)
    return false;
  PixelData->SetPhotometricInterpretation(pi);
  // Planar Configuration
  const DataElement & planarconfiguration_de = ds.GetDataElement(Tag(0x0028, 0x0006));
  if (!planarconfiguration_de.IsEmpty())
  {
    Attribute<0x0028, 0x0006> at = { 0 };
    at.SetFromDataElement(planarconfiguration_de);
    unsigned int pc = at.GetValue();
    if (pc &&
        (!(PixelData->GetPixelFormat().GetSamplesPerPixel() == 3 ||
           PixelData->GetPixelFormat().GetSamplesPerPixel() == 4)))
    {
      mdcmDebugMacro("Cannot have PlanarConfiguration 1");
      pc = 0;
    }
    PixelData->SetPlanarConfiguration(pc);
  }
  //
  // Pixel padding
  // bool pixelpaddingvalue = ds.FindDataElement(Tag(0x0028,0x0120));
  //
  // Palette Color Lookup Table Descriptor
  if (pi == PhotometricInterpretation::PALETTE_COLOR)
  {
    LookupTable lut;
    lut.Allocate(pf.GetBitsAllocated());
    for (int i = 0; i < 3; ++i)
    {
      const Tag                tdescriptor(0x0028, static_cast<uint16_t>(0x1101 + i));
      Element<VR::US, VM::VM3> el_us3 = { { 0, 0, 0 } };
      el_us3.SetFromDataElement(ds[tdescriptor]);
      lut.InitializeLUT(LookupTable::LookupTableType(i), el_us3[0], el_us3[1], el_us3[2]);
      const DataElement & lut_de = ds.GetDataElement(Tag(0x0028, static_cast<uint16_t>(0x1201 + i)));
      if (!lut_de.IsEmpty())
      {
        const ByteValue * lut_raw = lut_de.GetByteValue();
        if (lut_raw)
        {
          lut.SetLUT(
            LookupTable::LookupTableType(i),
            reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
            lut_raw->GetLength());
          unsigned int check = (el_us3.GetValue(0) ? el_us3.GetValue(0) : 65536) * el_us3.GetValue(2) / 8;
          assert(!lut.Initialized() || check == lut_raw->GetLength());
          (void)check;
        }
        else
        {
          return false;
        }
      }
      else
      {
        const DataElement & seglut_de = ds.GetDataElement(Tag(0x0028, static_cast<uint16_t>(0x1221 + i)));
        if (!seglut_de.IsEmpty())
        {
          const ByteValue * lut_raw = seglut_de.GetByteValue();
          if (lut_raw)
          {
            lut.SetSegmentedLUT(
              LookupTable::LookupTableType(i),
              reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
              lut_raw->GetLength());
          }
          else
          {
            return false;
          }
        }
        else
        {
          return false;
        }
      }
    }
    if (!lut.Initialized())
      return false;
    PixelData->SetLUT(std::move(lut));
  }
  // Supplemental LUT
  else if (pi == PhotometricInterpretation::MONOCHROME2 && m_AlppySupplementalLUT)
  {
    LookupTable lut;
    lut.Allocate(pf.GetBitsAllocated());
    bool lut_ok = true;
    for (int i = 0; i < 3; ++i)
    {
      const Tag                tdescriptor(0x0028, static_cast<uint16_t>(0x1101 + i));
      Element<VR::US, VM::VM3> el_us3 = { { 0, 0, 0 } };
      // Now pass the byte array to a DICOMizer
      el_us3.SetFromDataElement(ds[tdescriptor]);
      lut.InitializeLUT(LookupTable::LookupTableType(i), el_us3[0], el_us3[1], el_us3[2]);
      const DataElement & lut_de = ds.GetDataElement(Tag(0x0028, static_cast<uint16_t>(0x1201 + i)));
      if (!lut_de.IsEmpty())
      {
        const ByteValue * lut_raw = lut_de.GetByteValue();
        if (lut_raw)
        {
          // LookupTableType::SUPPLRED == 4
          lut.SetLUT(
            LookupTable::LookupTableType(i + 4),
            reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
            lut_raw->GetLength());
          unsigned int check = (el_us3.GetValue(0) ? el_us3.GetValue(0) : 65536) * el_us3.GetValue(2) / 8;
          assert(!lut.Initialized() || check == lut_raw->GetLength());
          (void)check;
        }
        else
        {
          lut_ok = false;
          break;
        }
      }
      else
      {
        const DataElement & seglut_de = ds.GetDataElement(Tag(0x0028, static_cast<uint16_t>(0x1221 + i)));
        if (!seglut_de.IsEmpty())
        {
          const ByteValue * lut_raw = seglut_de.GetByteValue();
          if (lut_raw)
          {
            lut.SetSegmentedLUT(
              LookupTable::LookupTableType(i + 4),
              reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
              lut_raw->GetLength());
          }
          else
          {
            lut_ok = false;
            break;
          }
        }
        else
        {
          lut_ok = false;
          break;
        }
      }
    }
    if (lut_ok && lut.Initialized())
    {
      PixelData->SetLUT(std::move(lut));
    }
    else
    {
      lut.Clear();
    }
  }
  // IconImage
  if (m_ProcessIcons)
    DoIconImage(ds, *PixelData);
  // Curves
  if (m_ProcessCurves)
    DoCurves(ds, *PixelData);
  // Overlays
  if (m_ProcessOverlays)
    DoOverlays(ds, *PixelData);
  // PixelData
  if (handlepixeldata)
  {
    if (pixeldata_)
    {
      const DataElement & xde = ds.GetDataElement(pixeldata);
      bool                need = PixelData->GetTransferSyntax() == TransferSyntax::ImplicitVRBigEndianPrivateGE;
      PixelData->SetNeedByteSwap(need);
      PixelData->SetDataElement(xde);
    }
    else if (pixeldataf_)
    {
      const DataElement & xde = ds.GetDataElement(pixeldataf);
      PixelData->SetNeedByteSwap(false);
      PixelData->SetDataElement(xde);
    }
    else if (pixeldatad_)
    {
      const DataElement & xde = ds.GetDataElement(pixeldatad);
      PixelData->SetNeedByteSwap(false);
      PixelData->SetDataElement(xde);
    }
    else
    {
      mdcmWarningMacro("No Pixel Data Found");
      return false;
    }
    // FIXME
    // We should check that when PixelData is RAW that Col * Dim == PixelData->GetLength()
    const unsigned int * dims = PixelData->GetDimensions();
    if (dims[0] == 0 || dims[1] == 0)
    {
      // Pseudo-declared JPEG SC image storage. Let's fix col/row/pf/pi
      JPEGCodec jpeg;
      if (jpeg.CanDecode(PixelData->GetTransferSyntax()))
      {
        std::stringstream           ss;
        const DataElement &         de = PixelData->GetDataElement();
        const SequenceOfFragments * sqf = de.GetSequenceOfFragments();
        if (!sqf)
        {
          mdcmDebugMacro("File is declared as JPEG compressed but does not contains Fragments");
          return false;
        }
        sqf->WriteBuffer(ss);
        PixelFormat jpegpf(PixelFormat::UINT8); // guess
        jpeg.SetPixelFormat(jpegpf);
        const bool b = jpeg.GetHeaderInfo(ss);
        if (b)
        {
          std::vector<unsigned int> v(3);
          v[0] = PixelData->GetDimensions()[0];
          v[1] = PixelData->GetDimensions()[1];
          v[2] = PixelData->GetDimensions()[2];
          assert(jpeg.GetDimensions()[0]);
          assert(jpeg.GetDimensions()[1]);
          v[0] = jpeg.GetDimensions()[0];
          v[1] = jpeg.GetDimensions()[1];
          PixelData->SetDimensions(v.data());
          if (PixelData->GetPixelFormat().GetSamplesPerPixel() != jpeg.GetPixelFormat().GetSamplesPerPixel())
          {
            mdcmDebugMacro("Fix samples per pixel.");
            PixelData->GetPixelFormat().SetSamplesPerPixel(jpeg.GetPixelFormat().GetSamplesPerPixel());
          }
          if (PixelData->GetPhotometricInterpretation().GetSamplesPerPixel() !=
              jpeg.GetPhotometricInterpretation().GetSamplesPerPixel())
          {
            mdcmDebugMacro("Fix photometric interpretation.");
            PixelData->SetPhotometricInterpretation(jpeg.GetPhotometricInterpretation());
          }
        }
        else
        {
          mdcmDebugMacro("Columns or Row was found to be 0. Cannot compute dimension.");
          return false;
        }
      }
      else
      {
        mdcmDebugMacro("Columns or Row was found to be 0. Cannot compute dimension.");
        return false;
      }
    }
  }
  // LossyImageCompression
  Attribute<0x0028, 0x2110> licat;
  bool                      lossyflag = false;
  bool                      haslossyflag = false;
  if (ds.FindDataElement(licat.GetTag()))
  {
    haslossyflag = true;
    licat.SetFromDataSet(ds); // could be empty
    const CSComp & v = licat.GetValue();
    lossyflag = atoi(v.c_str()) == 1;
    // Note: technically one can decompress into uncompressed form (eg.
    // Implicit Little Endian) an input JPEG Lossy. So we need to check
    // the attribute LossyImageCompression value:
    PixelData->SetLossyFlag(lossyflag);
  }
  // Two cases:
  // - DataSet did not specify the lossyflag
  // - DataSet specify it to be 0, but there is still a chance it could be wrong
  if (!haslossyflag || !lossyflag)
  {
    PixelData->ComputeLossyFlag();
    if (PixelData->IsLossy() && (!lossyflag && haslossyflag))
    {
      mdcmWarningMacro("DataSet set LossyFlag to 0, while Codec made the stream lossy");
    }
  }
  return true;
}

bool
PixmapReader::ReadImage(const MediaStorage & ms)
{
  return ReadImageInternal(ms);
}

bool
PixmapReader::ReadACRNEMAImage()
{
  const DataSet &   ds = F->GetDataSet();
  // Ok we have the dataset let's feed the Image (PixelData)
  // First find how many dimensions there is:
  // D 0028|0005 [SS] [Image Dimensions (RET)] [2]
  const DataElement & imagedimensions_de = ds.GetDataElement(Tag(0x0028, 0x0005));
  if (!imagedimensions_de.IsEmpty())
  {
    Attribute<0x0028, 0x0005> at0 = { 0 };
    at0.SetFromDataElement(imagedimensions_de);
    assert(at0.GetNumberOfValues() == 1);
    unsigned short imagedimensions = at0.GetValue();
    if (imagedimensions == 3)
    {
      PixelData->SetNumberOfDimensions(3);
      const DataElement &       de1 = ds.GetDataElement(Tag(0x0028, 0x0012));
      Attribute<0x0028, 0x0012> at1 = { 0 };
      at1.SetFromDataElement(de1);
      assert(at1.GetNumberOfValues() == 1);
      PixelData->SetDimension(2, at1.GetValue());
    }
    else if (imagedimensions == 2)
    {
      PixelData->SetNumberOfDimensions(2);
    }
    else
    {
      mdcmErrorMacro("Unhandled Image Dimensions: " << imagedimensions);
      return false;
    }
  }
  else
  {
    mdcmWarningMacro("Attempting a guess for the number of dimensions");
    PixelData->SetNumberOfDimensions(2);
  }
  {
    Attribute<0x0028, 0x0011> at = { 0 };
    at.SetFromDataSet(ds);
    PixelData->SetDimension(0, at.GetValue());
  }
  {
    Attribute<0x0028, 0x0010> at = { 0 };
    at.SetFromDataSet(ds);
    PixelData->SetDimension(1, at.GetValue());
  }
  // This is the definition of an ACR NEMA image:
  // D 0008|0010 [LO] [Recognition Code (RET)] [ACR-NEMA 2.0]
  // LIBIDO compatible code:
  // D 0008|0010 [LO] [Recognition Code (RET)] [ACRNEMA_LIBIDO_1.1]
  const DataElement recognitioncode_de = ds.GetDataElement(Tag(0x0008, 0x0010));
  if (!recognitioncode_de.IsEmpty())
  {
    const ByteValue * libido = recognitioncode_de.GetByteValue();
    assert(libido);
    if (libido)
    {
      std::string libido_str(libido->GetPointer(), libido->GetLength());
      assert(libido_str != "CANRME_AILIBOD1_1.");
      if (strcmp(libido_str.c_str(), "ACRNEMA_LIBIDO_1.1") == 0 || strcmp(libido_str.c_str(), "ACRNEMA_LIBIDO_1.0") == 0)
      {
        // Swap Columns & Rows
        const unsigned int * dims = PixelData->GetDimensions();
        unsigned int         tmp = dims[0];
        PixelData->SetDimension(0, dims[1]);
        PixelData->SetDimension(1, tmp);
      }
      else
      {
        assert(libido_str == "ACR-NEMA 2.0" || libido_str == "ACR-NEMA 1.0");
      }
    }
  }
  else
  {
    mdcmWarningMacro("Reading as ACR NEMA an image which does not look likes ACR NEMA");
  }
  PixelFormat pf;
  {
    Attribute<0x0028, 0x0100> at = { 0 };
    at.SetFromDataSet(ds);
    pf.SetBitsAllocated(at.GetValue());
  }
  {
    Attribute<0x0028, 0x0101> at = { 0 };
    at.SetFromDataSet(ds);
    pf.SetBitsStored(at.GetValue());
  }
  {
    Attribute<0x0028, 0x0102> at = { 0 };
    at.SetFromDataSet(ds);
    pf.SetHighBit(at.GetValue());
  }
  {
    Attribute<0x0028, 0x0103> at = { 0 };
    at.SetFromDataSet(ds);
    pf.SetPixelRepresentation(at.GetValue());
  }
  PixelData->SetPixelFormat(pf);
  // Curves
  if (m_ProcessCurves)
    DoCurves(ds, *PixelData);
  // Overlays
  if (m_ProcessOverlays)
    DoOverlays(ds, *PixelData);
  // PixelData
  const Tag pixeldata = Tag(0x7fe0, 0x0010);
  const DataElement & de = ds.GetDataElement(pixeldata);
  if (de.IsEmpty())
  {
    mdcmWarningMacro("No Pixel Data Found");
    return false;
  }
  PixelData->SetDataElement(de);
  PixelData->SetPhotometricInterpretation(PhotometricInterpretation::MONOCHROME2);
  PixelData->SetPlanarConfiguration(0);
  const Tag planarconfiguration(0x0028, 0x0006);
  const DataElement & planarconfiguration_de = ds.GetDataElement(planarconfiguration);
  if (!planarconfiguration_de.IsEmpty())
  {
    Attribute<0x0028, 0x0006> at = { 0 };
    at.SetFromDataSet(ds);
    unsigned int pc = at.GetValue();
    if (pc && PixelData->GetPixelFormat().GetSamplesPerPixel() != 3)
    {
      mdcmDebugMacro("Cannot have PlanarConfiguration=1, when Sample Per Pixel != 3");
      pc = 0;
    }
    PixelData->SetPlanarConfiguration(pc);
  }
  const Tag tphotometricinterpretation(0x0028, 0x0004);
  const DataElement & photometricinterpretation_de = ds.GetDataElement(tphotometricinterpretation);
  // Some funny ACR NEMA file have PhotometricInterpretation
  if (!photometricinterpretation_de.IsEmpty())
  {
    const ByteValue * photometricinterpretation = photometricinterpretation_de.GetByteValue();
    assert(photometricinterpretation);
    std::string               photometricinterpretation_str(photometricinterpretation->GetPointer(),
                                                            photometricinterpretation->GetLength());
    PhotometricInterpretation pi(PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str()));
    PixelData->SetPhotometricInterpretation(pi);
  }
  else
  {
    if (PixelData->GetPixelFormat().GetSamplesPerPixel() == 1)
    {
      assert(PixelData->GetPhotometricInterpretation() == PhotometricInterpretation::MONOCHROME2);
    }
    else if (PixelData->GetPixelFormat().GetSamplesPerPixel() == 3)
    {
      PixelData->SetPhotometricInterpretation(PhotometricInterpretation::RGB);
    }
    else if (PixelData->GetPixelFormat().GetSamplesPerPixel() == 4)
    {
      PixelData->SetPhotometricInterpretation(PhotometricInterpretation::ARGB);
    }
    else
    {
      mdcmErrorMacro("Cannot handle Samples Per Pixel=" << PixelData->GetPixelFormat().GetSamplesPerPixel());
      return false;
    }
  }
  return true;
}

} // end namespace mdcm
