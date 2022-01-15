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

#include "mdcmPixmapWriter.h"
#include "mdcmImageHelper.h"
#include "mdcmTrace.h"
#include "mdcmDataSet.h"
#include "mdcmDataElement.h"
#include "mdcmAttribute.h"
#include "mdcmUIDGenerator.h"
#include "mdcmSystem.h"
#include "mdcmPixmap.h"
#include "mdcmLookupTable.h"
#include "mdcmItem.h"
#include "mdcmSequenceOfItems.h"

namespace mdcm
{

PixmapWriter::PixmapWriter()
  : PixelData(new Pixmap)
{}

PixmapWriter::~PixmapWriter() {}

const Pixmap &
PixmapWriter::GetPixmap() const
{
  return *PixelData;
}

Pixmap &
PixmapWriter::GetPixmap() // FIXME
{
  return *PixelData;
}

void
PixmapWriter::SetPixmap(Pixmap const & img)
{
  PixelData = img;
}

const Pixmap &
PixmapWriter::GetImage() const
{
  return *PixelData;
}

Pixmap &
PixmapWriter::GetImage() // FIXME
{
  return *PixelData;
}

void
PixmapWriter::DoIconImage(DataSet & rootds, Pixmap const & image)
{
  const IconImage & icon = image.GetIconImage();
  if (!icon.IsEmpty())
  {
    DataElement iconimagesq;
    iconimagesq.SetTag(Attribute<0x0088, 0x0200>::GetTag());
    iconimagesq.SetVR(VR::SQ);
    SmartPointer<SequenceOfItems> sq = new SequenceOfItems;
    sq->SetLengthToUndefined();
    DataSet                   ds;
    Attribute<0x0028, 0x0011> columns;
    columns.SetValue((uint16_t)icon.GetDimension(0));
    ds.Insert(columns.GetAsDataElement());
    Attribute<0x0028, 0x0010> rows;
    rows.SetValue((uint16_t)icon.GetDimension(1));
    ds.Insert(rows.GetAsDataElement());
    PixelFormat               pf = icon.GetPixelFormat();
    Attribute<0x0028, 0x0100> bitsallocated;
    bitsallocated.SetValue(pf.GetBitsAllocated());
    ds.Replace(bitsallocated.GetAsDataElement());

    Attribute<0x0028, 0x0101> bitsstored;
    bitsstored.SetValue(pf.GetBitsStored());
    ds.Replace(bitsstored.GetAsDataElement());

    Attribute<0x0028, 0x0102> highbit;
    highbit.SetValue(pf.GetHighBit());
    ds.Replace(highbit.GetAsDataElement());

    Attribute<0x0028, 0x0103> pixelrepresentation;
    pixelrepresentation.SetValue(pf.GetPixelRepresentation());
    ds.Replace(pixelrepresentation.GetAsDataElement());

    Attribute<0x0028, 0x0002> samplesperpixel;
    samplesperpixel.SetValue(pf.GetSamplesPerPixel());
    ds.Replace(samplesperpixel.GetAsDataElement());

    if (pf.GetSamplesPerPixel() != 1)
    {
      Attribute<0x0028, 0x0006> planarconf;
      planarconf.SetValue((uint16_t)icon.GetPlanarConfiguration());
      ds.Replace(planarconf.GetAsDataElement());
    }
    PhotometricInterpretation pi = icon.GetPhotometricInterpretation();
    Attribute<0x0028, 0x0004> piat;
    const char *              pistr = PhotometricInterpretation::GetPIString(pi);
    {
      DataElement de(Tag(0x0028, 0x0004));
      VL::Type    strlenPistr = (VL::Type)strlen(pistr);
      de.SetByteValue(pistr, strlenPistr);
      de.SetVR(piat.GetVR());
      ds.Replace(de);
    }
    if (pi == PhotometricInterpretation::PALETTE_COLOR)
    {
      const LookupTable & lut = icon.GetLUT();
      assert((pf.GetBitsAllocated() == 8 && pf.GetPixelRepresentation() == 0) ||
             (pf.GetBitsAllocated() == 16 && pf.GetPixelRepresentation() == 0));
      unsigned short              length, subscript, bitsize;
      std::vector<unsigned short> rawlut8;
      rawlut8.resize(256);
      std::vector<unsigned short> rawlut16;
      unsigned short *            rawlut = &rawlut8[0];
      unsigned int                lutlen = 256;
      if (pf.GetBitsAllocated() == 16)
      {
        rawlut16.resize(65536);
        rawlut = &rawlut16[0];
        lutlen = 65536;
      }
      unsigned int l;
      // RED
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::RED, (unsigned char *)rawlut, l);
      DataElement redde(Tag(0x0028, 0x1201));
      redde.SetVR(VR::OW);
      redde.SetByteValue((char *)rawlut, l);
      ds.Replace(redde);
      // Descriptor
      Attribute<0x0028, 0x1101, VR::US, VM::VM3> reddesc;
      lut.GetLUTDescriptor(LookupTable::RED, length, subscript, bitsize);
      reddesc.SetValue(length, 0);
      reddesc.SetValue(subscript, 1);
      reddesc.SetValue(bitsize, 2);
      ds.Replace(reddesc.GetAsDataElement());
      // GREEN
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::GREEN, (unsigned char *)rawlut, l);
      DataElement greende(Tag(0x0028, 0x1202));
      greende.SetVR(VR::OW);
      greende.SetByteValue((char *)rawlut, l);
      ds.Replace(greende);
      // Descriptor
      Attribute<0x0028, 0x1102, VR::US, VM::VM3> greendesc;
      lut.GetLUTDescriptor(LookupTable::GREEN, length, subscript, bitsize);
      greendesc.SetValue(length, 0);
      greendesc.SetValue(subscript, 1);
      greendesc.SetValue(bitsize, 2);
      ds.Replace(greendesc.GetAsDataElement());
      // BLUE
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::BLUE, (unsigned char *)rawlut, l);
      DataElement bluede(Tag(0x0028, 0x1203));
      bluede.SetVR(VR::OW);
      bluede.SetByteValue((char *)rawlut, l);
      ds.Replace(bluede);
      // Descriptor
      Attribute<0x0028, 0x1103, VR::US, VM::VM3> bluedesc;
      lut.GetLUTDescriptor(LookupTable::BLUE, length, subscript, bitsize);
      bluedesc.SetValue(length, 0);
      bluedesc.SetValue(subscript, 1);
      bluedesc.SetValue(bitsize, 2);
      ds.Replace(bluedesc.GetAsDataElement());
    }
    {
      DataElement   de(Tag(0x7fe0, 0x0010));
      const Value & v = icon.GetDataElement().GetValue();
      de.SetValue(v);
      const ByteValue *      bv = de.GetByteValue();
      const TransferSyntax & ts = icon.GetTransferSyntax();
      assert(ts.IsExplicit() || ts.IsImplicit());
      VL vl;
      if (bv)
      {
        vl = bv->GetLength();
      }
      else
      {
        vl.SetToUndefined();
      }
      if (ts.IsExplicit())
      {
        switch (pf.GetBitsAllocated())
        {
          case 8:
            de.SetVR(VR::OB);
            break;
          case 16:
          case 32:
            de.SetVR(VR::OW);
            break;
          default:
            assert(0 && "should not happen");
            break;
        }
      }
      else
      {
        de.SetVR(VR::OB);
      }
      de.SetVL(vl);
      ds.Replace(de);
    }
    Item item;
    item.SetNestedDataSet(ds);
    sq->AddItem(item);
    iconimagesq.SetValue(*sq);
    rootds.Replace(iconimagesq);
  }
}

bool
PixmapWriter::PrepareWrite(MediaStorage const & ref_ms)
{
  File &                 file = GetFile();
  DataSet &              ds = file.GetDataSet();
  FileMetaInformation &  fmi_orig = file.GetHeader();
  const TransferSyntax & ts_orig = fmi_orig.GetDataSetTransferSyntax();
  PixelFormat            pf = PixelData->GetPixelFormat();
  if (!pf.IsValid())
  {
    mdcmAlwaysWarnMacro("Pixel format is not valid: " << pf);
    return false;
  }
  PhotometricInterpretation pi = PixelData->GetPhotometricInterpretation();
  if (pi.GetSamplesPerPixel() != pf.GetSamplesPerPixel())
  {
    mdcmAlwaysWarnMacro("Photometric Interpretation and Pixel format are not compatible: "
                     << pi.GetSamplesPerPixel() << " vs " << pf.GetSamplesPerPixel());
    return false;
  }
  {
    assert(pi != PhotometricInterpretation::UNKNOWN);
    const char * pistr = PhotometricInterpretation::GetPIString(pi);
    DataElement  de(Tag(0x0028, 0x0004));
    VL::Type     strlenPistr = (VL::Type)strlen(pistr);
    de.SetByteValue(pistr, strlenPistr);
    de.SetVR(Attribute<0x0028, 0x0004>::GetVR());
    ds.Replace(de);
  }
  Attribute<0x0028, 0x0100> bitsallocated;
  bitsallocated.SetValue(pf.GetBitsAllocated());
  ds.Replace(bitsallocated.GetAsDataElement());
  Attribute<0x0028, 0x0101> bitsstored;
  bitsstored.SetValue(pf.GetBitsStored());
  ds.Replace(bitsstored.GetAsDataElement());
  Attribute<0x0028, 0x0102> highbit;
  highbit.SetValue(pf.GetHighBit());
  ds.Replace(highbit.GetAsDataElement());
  Attribute<0x0028, 0x0103> pixelrepresentation;
  pixelrepresentation.SetValue(pf.GetPixelRepresentation());
  ds.Replace(pixelrepresentation.GetAsDataElement());
  Attribute<0x0028, 0x0002> samplesperpixel;
  samplesperpixel.SetValue(pf.GetSamplesPerPixel());
  ds.Replace(samplesperpixel.GetAsDataElement());
  if (pf.GetSamplesPerPixel() != 1)
  {
    Attribute<0x0028, 0x0006> planarconf;
    planarconf.SetValue((uint16_t)PixelData->GetPlanarConfiguration());
    ds.Replace(planarconf.GetAsDataElement());
  }
  {
    if (pi == PhotometricInterpretation::PALETTE_COLOR)
    {
      const LookupTable & lut = PixelData->GetLUT();
      assert(lut.Initialized());
      unsigned short   length, subscript, bitsize;
      unsigned short   rawlut8[256];
      unsigned short   rawlut16[65536];
      unsigned short * rawlut = rawlut8;
      unsigned int     lutlen = 256;
      if (pf.GetBitsAllocated() == 16)
      {
        rawlut = rawlut16;
        lutlen = 65536;
      }
      unsigned int l;
      // RED
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::RED, (unsigned char *)rawlut, l);
      DataElement redde(Tag(0x0028, 0x1201));
      redde.SetVR(VR::OW);
      redde.SetByteValue((char *)rawlut, l);
      ds.Replace(redde);
      // Descriptor
      Attribute<0x0028, 0x1101, VR::US, VM::VM3> reddesc;
      lut.GetLUTDescriptor(LookupTable::RED, length, subscript, bitsize);
      reddesc.SetValue(length, 0);
      reddesc.SetValue(subscript, 1);
      reddesc.SetValue(bitsize, 2);
      ds.Replace(reddesc.GetAsDataElement());
      // GREEN
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::GREEN, (unsigned char *)rawlut, l);
      DataElement greende(Tag(0x0028, 0x1202));
      greende.SetVR(VR::OW);
      greende.SetByteValue((char *)rawlut, l);
      ds.Replace(greende);
      // Descriptor
      Attribute<0x0028, 0x1102, VR::US, VM::VM3> greendesc;
      lut.GetLUTDescriptor(LookupTable::GREEN, length, subscript, bitsize);
      greendesc.SetValue(length, 0);
      greendesc.SetValue(subscript, 1);
      greendesc.SetValue(bitsize, 2);
      ds.Replace(greendesc.GetAsDataElement());
      // BLUE
      memset(rawlut, 0, lutlen * 2);
      lut.GetLUT(LookupTable::BLUE, (unsigned char *)rawlut, l);
      DataElement bluede(Tag(0x0028, 0x1203));
      bluede.SetVR(VR::OW);
      bluede.SetByteValue((char *)rawlut, l);
      ds.Replace(bluede);
      // Descriptor
      Attribute<0x0028, 0x1103, VR::US, VM::VM3> bluedesc;
      lut.GetLUTDescriptor(LookupTable::BLUE, length, subscript, bitsize);
      bluedesc.SetValue(length, 0);
      bluedesc.SetValue(subscript, 1);
      bluedesc.SetValue(bitsize, 2);
      ds.Replace(bluedesc.GetAsDataElement());
    }
    ds.Remove(Tag(0x0028, 0x1221));
    ds.Remove(Tag(0x0028, 0x1222));
    ds.Remove(Tag(0x0028, 0x1223));
  }
  // Cleanup LUT here since cant be done within mdcm::ImageApplyLookupTable
  if (pi == PhotometricInterpretation::RGB)
  {
    ds.Remove(Tag(0x0028, 0x1101));
    ds.Remove(Tag(0x0028, 0x1102));
    ds.Remove(Tag(0x0028, 0x1103));
    ds.Remove(Tag(0x0028, 0x1201));
    ds.Remove(Tag(0x0028, 0x1202));
    ds.Remove(Tag(0x0028, 0x1203));
    ds.Remove(Tag(0x0028, 0x1221));
    ds.Remove(Tag(0x0028, 0x1222));
    ds.Remove(Tag(0x0028, 0x1223));
    ds.Remove(Tag(0x0028, 0x1199));
  }
  // Overlay Data 60xx
  SequenceOfItems::SizeType nOv = PixelData->GetNumberOfOverlays();
  for (SequenceOfItems::SizeType ovidx = 0; ovidx < nOv; ++ovidx)
  {
    DataElement               de;
    const Overlay &           ov = PixelData->GetOverlay(ovidx);
    Attribute<0x6000, 0x0010> overlayrows;
    overlayrows.SetValue(ov.GetRows());
    de = overlayrows.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    Attribute<0x6000, 0x0011> overlaycolumns;
    overlaycolumns.SetValue(ov.GetColumns());
    de = overlaycolumns.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    if (ov.GetDescription()) // Type 3
    {
      Attribute<0x6000, 0x0022> overlaydescription;
      overlaydescription.SetValue(ov.GetDescription());
      de = overlaydescription.GetAsDataElement();
      de.GetTag().SetGroup(ov.GetGroup());
      ds.Replace(de);
    }
    Attribute<0x6000, 0x0040> overlaytype; // 'G' or 'R'
    overlaytype.SetValue(ov.GetType());
    de = overlaytype.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    Attribute<0x6000, 0x0050> overlayorigin;
    overlayorigin.SetValues(ov.GetOrigin());
    de = overlayorigin.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    Attribute<0x6000, 0x0100> overlaybitsallocated;
    overlaybitsallocated.SetValue(ov.GetBitsAllocated());
    de = overlaybitsallocated.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    Attribute<0x6000, 0x0102> overlaybitposition;
    overlaybitposition.SetValue(ov.GetBitPosition());
    de = overlaybitposition.GetAsDataElement();
    de.GetTag().SetGroup(ov.GetGroup());
    ds.Replace(de);
    // FIXME: for now rewrite 'Overlay in pixel data' still in the pixel data element
    // if(!ov.IsInPixelData())
    {
      const ByteValue & overlaydatabv = ov.GetOverlayData();
      DataElement       overlaydata(Tag(0x6000, 0x3000));
      overlaydata.SetByteValue(overlaydatabv.GetPointer(), overlaydatabv.GetLength());
      overlaydata.SetVR(VR::OW); // FIXME
      overlaydata.GetTag().SetGroup(ov.GetGroup());
      ds.Replace(overlaydata);
    }
  }
  // Pixel Data
  DataElement       depixdata(Tag(0x7fe0, 0x0010));
  DataElement &     pde = PixelData->GetDataElement();
  const ByteValue * bvpixdata = NULL;
  if (!pde.IsEmpty())
  {
    const Value & v = PixelData->GetDataElement().GetValue();
    depixdata.SetValue(v);
    bvpixdata = depixdata.GetByteValue();
  }
  const TransferSyntax & ts = PixelData->GetTransferSyntax();
  assert(ts.IsExplicit() || ts.IsImplicit());
  if (PixelData->IsLossy())
  {
    Attribute<0x0028, 0x2110> at1;
    Attribute<0x0028, 0x2114> at3;
    if (ts_orig == TransferSyntax::TS_END)
    {
      at1.SetValue("01");
      ds.Replace(at1.GetAsDataElement());
    }
    else if (ts_orig.IsLossy())
    {
      at1.SetValue("01");
      ds.Replace(at1.GetAsDataElement());
      if (ts_orig == TransferSyntax::JPEG2000)
      {
        static const CSComp newvalues2[] = { "ISO_15444_1" };
        at3.SetValues(newvalues2, 1);
      }
      else if (ts_orig == TransferSyntax::JPEGLSNearLossless)
      {
        static const CSComp newvalues2[] = { "ISO_14495_1" };
        at3.SetValues(newvalues2, 1);
      }
      else if (ts_orig == TransferSyntax::JPEGBaselineProcess1 ||
               ts_orig == TransferSyntax::JPEGExtendedProcess2_4 ||
               ts_orig == TransferSyntax::JPEGExtendedProcess3_5 ||
               ts_orig == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
               ts_orig == TransferSyntax::JPEGFullProgressionProcess10_12)
      {
        static const CSComp newvalues2[] = { "ISO_10918_1" };
        at3.SetValues(newvalues2, 1);
      }
      else
      {
        mdcmAlwaysWarnMacro("Pixel Data is lossy but I cannot find the original transfer syntax");
        return false;
      }
      ds.Replace(at3.GetAsDataElement());
    }
    else
    {
      if (ds.FindDataElement(at1.GetTag()))
      {
        at1.Set(ds);
        if (atoi(at1.GetValue().c_str()) != 1)
        {
          mdcmWarningMacro("Invalid value for LossyImageCompression");
        }
      }
      else
      {
        mdcmWarningMacro("Missing attribute for LossyImageCompression");
      }
    }
  }
  VL vl;
  if (bvpixdata)
  {
    vl = bvpixdata->GetLength();
  }
  else
  {
    vl.SetToUndefined();
  }
  if (ts.IsExplicit())
  {
    switch (pf.GetBitsAllocated())
    {
      case 1:
      case 8:
        depixdata.SetVR(VR::OB);
        break;
      case 12:
      case 16:
      case 32:
        if (depixdata.GetSequenceOfFragments())
        {
          depixdata.SetVR(VR::OB);
        }
        else
        {
          depixdata.SetVR(VR::OW);
        }
        break;
      default:
        assert(0 && "should not happen");
        break;
    }
  }
  else
  {
    depixdata.SetVR(VR::OB);
  }
  depixdata.SetVL(vl);
  if (!pde.IsEmpty())
  {
    ds.Replace(depixdata);
  }
  DoIconImage(ds, GetPixmap());
  MediaStorage ms = ref_ms;
  // Make sure 3D is ok
  if (PixelData->GetNumberOfDimensions() > 2)
  {
    if (ms.GetModalityDimension() < PixelData->GetNumberOfDimensions())
    {
      // Input was specified with SC, but the Number of Frames is > 1
      ms = ImageHelper::ComputeMediaStorageFromModality(ms.GetModality(),
                                                        PixelData->GetNumberOfDimensions(),
                                                        PixelData->GetPixelFormat(),
                                                        PixelData->GetPhotometricInterpretation(),
                                                        0,
                                                        1);
      if (ms.GetModalityDimension() < PixelData->GetNumberOfDimensions())
      {
        mdcmErrorMacro("Problem with NumberOfDimensions and MediaStorage");
        return false;
      }
    }
  }
  if (!(ms != MediaStorage::MS_END))
  {
    mdcmAlwaysWarnMacro("Internal error (101)");
    return false;
  }
  const char * msstr = MediaStorage::GetMSString(ms);
  if (!ds.FindDataElement(Tag(0x0008, 0x0016)))
  {
    DataElement de(Tag(0x0008, 0x0016));
    VL::Type    strlenMsstr = (VL::Type)strlen(msstr);
    de.SetByteValue(msstr, strlenMsstr);
    de.SetVR(Attribute<0x0008, 0x0016>::GetVR());
    ds.Insert(de);
  }
  else
  {
    const ByteValue * bv = ds.GetDataElement(Tag(0x0008, 0x0016)).GetByteValue();
    if (!bv)
    {
      mdcmErrorMacro("Can not be empty");
      return false;
    }
    if (strncmp(bv->GetPointer(), msstr, bv->GetLength()) != 0)
    {
      DataElement de = ds.GetDataElement(Tag(0x0008, 0x0016));
      VL::Type    strlenMsstr = (VL::Type)strlen(msstr);
      de.SetByteValue(msstr, strlenMsstr);
      ds.Replace(de);
    }
    else
    {
      assert(bv->GetLength() == strlen(msstr) || bv->GetLength() == strlen(msstr) + 1);
    }
  }
  ImageHelper::SetDimensionsValue(file, *PixelData);
  if (!GetSkipUIDs())
  {
    UIDGenerator uid;
    if (ds.FindDataElement(Tag(0x0008, 0x0018)) && false) // FIXME
    {
      // Reference
      const Tag                     tsourceImageSequence(0x0008, 0x2112);
      SmartPointer<SequenceOfItems> sq;
      if (ds.FindDataElement(tsourceImageSequence))
      {
        DataElement & de = const_cast<DataElement &>(ds.GetDataElement(tsourceImageSequence));
        de.SetVLToUndefined();
        if (de.IsEmpty())
        {
          sq = new SequenceOfItems;
          de.SetValue(*sq);
        }
        sq = de.GetValueAsSQ();
      }
      else
      {
        sq = new SequenceOfItems;
      }
      sq->SetLengthToUndefined();
      Item        item;
      DataElement referencedSOPClassUID = ds.GetDataElement(Tag(0x0008, 0x0016));
      referencedSOPClassUID.SetTag(Tag(0x0008, 0x1150));
      DataElement referencedSOPInstanceUID = ds.GetDataElement(Tag(0x0008, 0x0018));
      referencedSOPInstanceUID.SetTag(Tag(0x0008, 0x1155));
      item.SetVLToUndefined();
      item.InsertDataElement(referencedSOPClassUID);
      item.InsertDataElement(referencedSOPInstanceUID);
      sq->AddItem(item);
      if (!ds.FindDataElement(tsourceImageSequence))
      {
        DataElement de(tsourceImageSequence);
        de.SetVR(VR::SQ);
        de.SetValue(*sq);
        de.SetVLToUndefined();
        ds.Insert(de);
      }
    }
    {
      const char * sop = uid.Generate();
      DataElement  de(Tag(0x0008, 0x0018));
      VL::Type     strlenSOP = (VL::Type)strlen(sop);
      de.SetByteValue(sop, strlenSOP);
      de.SetVR(Attribute<0x0008, 0x0018>::GetVR());
      ds.ReplaceEmpty(de);
    }
    // Create a new UID
    if (!ds.FindDataElement(Tag(0x0020, 0x000d)))
    {
      const char * study = uid.Generate();
      DataElement  de(Tag(0x0020, 0x000d));
      VL::Type     strlenStudy = (VL::Type)strlen(study);
      de.SetByteValue(study, strlenStudy);
      de.SetVR(Attribute<0x0020, 0x000d>::GetVR());
      ds.ReplaceEmpty(de);
    }
    // Create a new UID
    if (!ds.FindDataElement(Tag(0x0020, 0x000e)))
    {
      const char * series = uid.Generate();
      DataElement  de(Tag(0x0020, 0x000e));
      VL::Type     strlenSeries = (VL::Type)strlen(series);
      de.SetByteValue(series, strlenSeries);
      de.SetVR(Attribute<0x0020, 0x000e>::GetVR());
      ds.ReplaceEmpty(de);
    }
  }
  FileMetaInformation & fmi = file.GetHeader();
  if (GetCheckFileMetaInformation())
  {
    fmi.Clear();
    {
      const char * tsuid = TransferSyntax::GetTSString(ts);
      DataElement  de(Tag(0x0002, 0x0010));
      VL::Type     strlenTSUID = (VL::Type)strlen(tsuid);
      de.SetByteValue(tsuid, strlenTSUID);
      de.SetVR(Attribute<0x0002, 0x0010>::GetVR());
      fmi.Replace(de);
      fmi.SetDataSetTransferSyntax(ts);
    }
    fmi.FillFromDataSet(ds);
  }
  else
  {
    Attribute<0x0002, 0x0010> at;
    at.SetFromDataSet(fmi);
    const char * tsuid = TransferSyntax::GetTSString(ts);
    UIComp       tsui = at.GetValue();
    if (tsui != tsuid)
    {
      mdcmErrorMacro("Incompatible TransferSyntax.");
      return false;
    }
  }
  return true;
}

void
PixmapWriter::SetImage(Pixmap const & img)
{
  PixelData = img;
}

bool
PixmapWriter::Write()
{
  MediaStorage ms;
  if (!ms.SetFromFile(GetFile()))
  {
    // Fix old ACR-NEMA stuff
    ms = ImageHelper::ComputeMediaStorageFromModality(ms.GetModality(),
                                                      PixelData->GetNumberOfDimensions(),
                                                      PixelData->GetPixelFormat(),
                                                      PixelData->GetPhotometricInterpretation(),
                                                      0,
                                                      1);
  }
  if (!PrepareWrite(ms))
    return false;
  assert(Stream);
  if (!Writer::Write())
  {
    return false;
  }
  return true;
}

} // end namespace mdcm
