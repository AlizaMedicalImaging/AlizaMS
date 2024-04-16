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
#include "mdcmIconImageFilter.h"
#include "mdcmIconImage.h"
#include "mdcmAttribute.h"
#include "mdcmPrivateTag.h"
#include "mdcmJPEGCodec.h"
#include <utility>

namespace mdcm
{

class IconImageFilterInternals
{
public:
  std::vector<SmartPointer<IconImage>> icons;
};

IconImageFilter::IconImageFilter()
  : F(new File)
  , Internals(new IconImageFilterInternals)
{}

IconImageFilter::~IconImageFilter()
{
  delete Internals;
}

void
IconImageFilter::SetFile(const File & f)
{
  F = f;
}

File &
IconImageFilter::GetFile()
{
  return *F;
}

const File &
IconImageFilter::GetFile() const
{
  return *F;
}

bool
IconImageFilter::Extract()
{
  Internals->icons.clear();
  ExtractIconImages();
  ExtractVeproIconImages();
  return (GetNumberOfIconImages() != 0);
}

unsigned int
IconImageFilter::GetNumberOfIconImages() const
{
  return static_cast<unsigned int>(Internals->icons.size());
}

IconImage &
IconImageFilter::GetIconImage(unsigned int i) const
{
  return *Internals->icons[i];
}

void
IconImageFilter::ExtractIconImages()
{
  const DataSet &             rootds = F->GetDataSet();
  const FileMetaInformation & header = F->GetHeader();
  const TransferSyntax &      ts = header.GetDataSetTransferSyntax();
  const Tag                   ticonimage(0x0088, 0x0200);
  if (rootds.FindDataElement(ticonimage))
  {
    const DataElement &           iconimagesq = rootds.GetDataElement(ticonimage);
    SmartPointer<SequenceOfItems> sq = iconimagesq.GetValueAsSQ();
    if (sq)
    {
      if (sq->GetNumberOfItems() != 1)
        return;
      SmartPointer<IconImage>        si1 = new IconImage;
      IconImage &                    pixeldata = *si1;
      SequenceOfItems::ConstIterator it = sq->Begin();
      const DataSet &                ds = it->GetNestedDataSet();
      {
        Attribute<0x0028, 0x0011> at = { 0 };
        at.SetFromDataSet(ds);
        pixeldata.SetDimension(0, at.GetValue());
      }
      {
        Attribute<0x0028, 0x0010> at = { 0 };
        at.SetFromDataSet(ds);
        pixeldata.SetDimension(1, at.GetValue());
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
      {
        Attribute<0x0028, 0x0002> at = { 1 };
        at.SetFromDataSet(ds);
        pf.SetSamplesPerPixel(at.GetValue());
      }
      pixeldata.SetPixelFormat(pf);
      const Tag tphotometricinterpretation(0x0028, 0x0004);
      if (!ds.FindDataElement(tphotometricinterpretation))
        return;
      const ByteValue * photometricinterpretation = ds.GetDataElement(tphotometricinterpretation).GetByteValue();
      if (!photometricinterpretation)
        return;
      std::string photometricinterpretation_str(photometricinterpretation->GetPointer(),
                                                photometricinterpretation->GetLength());
      PhotometricInterpretation pi(PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str()));
      pixeldata.SetPhotometricInterpretation(pi);
      if (pi == PhotometricInterpretation::PALETTE_COLOR)
      {
        LookupTable lut;
        lut.Allocate(pf.GetBitsAllocated());
        for (int i = 0; i < 3; ++i)
        {
          const Tag                tdescriptor(0x0028, static_cast<uint16_t>(0x1101 + i));
          Element<VR::US, VM::VM3> el_us3;
          el_us3.SetFromDataElement(ds[tdescriptor]);
          lut.InitializeLUT(LookupTable::LookupTableType(i), el_us3[0], el_us3[1], el_us3[2]);
          const Tag tlut(0x0028, static_cast<uint16_t>(0x1201 + i));
          const Tag seglut(0x0028, static_cast<uint16_t>(0x1221 + i));
          if (ds.FindDataElement(tlut))
          {
            const ByteValue * lut_raw = ds.GetDataElement(tlut).GetByteValue();
            if (lut_raw)
            {
              lut.SetLUT(
                LookupTable::LookupTableType(i),
                reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
                lut_raw->GetLength());
            }
          }
          else if (ds.FindDataElement(seglut))
          {
            const ByteValue * lut_raw = ds.GetDataElement(seglut).GetByteValue();
            if (lut_raw)
            {
              lut.SetSegmentedLUT(
                LookupTable::LookupTableType(i),
                reinterpret_cast<const unsigned char *>(lut_raw->GetPointer()),
                lut_raw->GetLength());
            }
          }
          else
          {
            mdcmWarningMacro("Icon Sequence is incomplete.");
            pixeldata.ClearDimensions();
            return;
          }
        }
        pixeldata.SetLUT(std::move(lut));
      }
      const Tag tpixeldata = Tag(0x7fe0, 0x0010);
      if (!ds.FindDataElement(tpixeldata))
      {
        mdcmWarningMacro("Icon Sequence is incomplete. Giving up");
        pixeldata.ClearDimensions();
        return;
      }
      const DataElement & de = ds.GetDataElement(tpixeldata);
      pixeldata.SetDataElement(de);
      pixeldata.SetTransferSyntax(ts);
      Internals->icons.push_back(pixeldata);
    }
  }
  const PrivateTag tgeiconimage(0x0009, 0x0010, "GEIIS");
  if (rootds.FindDataElement(tgeiconimage))
  {
    const DataElement &           iconimagesq = rootds.GetDataElement(tgeiconimage);
    SmartPointer<SequenceOfItems> sq = iconimagesq.GetValueAsSQ();
    if (!(sq && sq->GetNumberOfItems() > 0))
      return;
    SmartPointer<IconImage>        si1 = new IconImage;
    IconImage &                    pixeldata = *si1;
    SequenceOfItems::ConstIterator it = sq->Begin();
    const DataSet &                ds = it->GetNestedDataSet();
    {
      Attribute<0x0028, 0x0011> at = { 0 };
      at.SetFromDataSet(ds);
      pixeldata.SetDimension(0, at.GetValue());
    }
    {
      Attribute<0x0028, 0x0010> at = { 0 };
      at.SetFromDataSet(ds);
      pixeldata.SetDimension(1, at.GetValue());
    }
    PixelFormat pf1;
    {
      Attribute<0x0028, 0x0100> at = { 0 };
      at.SetFromDataSet(ds);
      pf1.SetBitsAllocated(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0101> at = { 0 };
      at.SetFromDataSet(ds);
      pf1.SetBitsStored(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0102> at = { 0 };
      at.SetFromDataSet(ds);
      pf1.SetHighBit(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0103> at = { 0 };
      at.SetFromDataSet(ds);
      pf1.SetPixelRepresentation(at.GetValue());
    }
    {
      Attribute<0x0028, 0x0002> at = { 1 };
      at.SetFromDataSet(ds);
      pf1.SetSamplesPerPixel(at.GetValue());
    }
    pixeldata.SetPixelFormat(pf1);
    const Tag tphotometricinterpretation(0x0028, 0x0004);
    if (!ds.FindDataElement(tphotometricinterpretation))
      return;
    const ByteValue * photometricinterpretation = ds.GetDataElement(tphotometricinterpretation).GetByteValue();
    if (!photometricinterpretation)
      return;
    std::string               photometricinterpretation_str(photometricinterpretation->GetPointer(),
                                              photometricinterpretation->GetLength());
    PhotometricInterpretation pi(PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str()));
    pixeldata.SetPhotometricInterpretation(pi);
    const Tag tpixeldata = Tag(0x7fe0, 0x0010);
    if (ds.FindDataElement(tpixeldata))
    {
      const DataElement & de = ds.GetDataElement(tpixeldata);
      std::istringstream  is;
      const ByteValue *   bv = de.GetByteValue();
      if (!bv)
        return;
      is.str(std::string(bv->GetPointer(), bv->GetLength()));
      TransferSyntax jpegts;
      JPEGCodec      jpeg;
      jpeg.SetPixelFormat(pf1);
      bool b = jpeg.GetHeaderInfoAndTS(is, jpegts);
      if (!b)
      {
        assert(0);
      }
      pixeldata.SetPixelFormat(jpeg.GetPixelFormat());
      pixeldata.SetTransferSyntax(jpegts);
      pixeldata.SetDataElement(de);
    }
    Internals->icons.push_back(pixeldata);
  }
  const PrivateTag tgeiconimage2(0x6003, 0x0010, "GEMS_Ultrasound_ImageGroup_001");
  if (rootds.FindDataElement(tgeiconimage2))
  {
    const DataElement &           iconimagesq = rootds.GetDataElement(tgeiconimage2);
    SmartPointer<SequenceOfItems> sq = iconimagesq.GetValueAsSQ();
    if (!(sq && sq->GetNumberOfItems() > 0))
      return;
    SmartPointer<IconImage>        si1 = new IconImage;
    IconImage &                    pixeldata = *si1;
    SequenceOfItems::ConstIterator it = sq->Begin();
    const DataSet &                ds = it->GetNestedDataSet();
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0011));
      Attribute<0x0028, 0x0011> at;
      at.SetFromDataElement(de);
      pixeldata.SetDimension(0, at.GetValue());
    }
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0010));
      Attribute<0x0028, 0x0010> at;
      at.SetFromDataElement(de);
      pixeldata.SetDimension(1, at.GetValue());
    }
    PixelFormat pf;
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0100));
      Attribute<0x0028, 0x0100> at;
      at.SetFromDataElement(de);
      pf.SetBitsAllocated(at.GetValue());
    }
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0101));
      Attribute<0x0028, 0x0101> at;
      at.SetFromDataElement(de);
      pf.SetBitsStored(at.GetValue());
    }
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0102));
      Attribute<0x0028, 0x0102> at;
      at.SetFromDataElement(de);
      pf.SetHighBit(at.GetValue());
    }
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0103));
      Attribute<0x0028, 0x0103> at;
      at.SetFromDataElement(de);
      pf.SetPixelRepresentation(at.GetValue());
    }
    {
      const DataElement &       de = ds.GetDataElement(Tag(0x0028, 0x0002));
      Attribute<0x0028, 0x0002> at;
      at.SetFromDataElement(de);
      pf.SetSamplesPerPixel(at.GetValue());
    }
    pixeldata.SetPixelFormat(pf);
    const Tag tphotometricinterpretation(0x0028, 0x0004);
    if (!ds.FindDataElement(tphotometricinterpretation))
      return;
    const ByteValue * photometricinterpretation = ds.GetDataElement(tphotometricinterpretation).GetByteValue();
    if (!photometricinterpretation)
      return;
    std::string               photometricinterpretation_str(photometricinterpretation->GetPointer(),
                                              photometricinterpretation->GetLength());
    PhotometricInterpretation pi(PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str()));
    pixeldata.SetPhotometricInterpretation(pi);
    const PrivateTag tpixeldata(0x6003, 0x0011, "GEMS_Ultrasound_ImageGroup_001");
    if (ds.FindDataElement(tpixeldata))
    {
      const DataElement & de = ds.GetDataElement(tpixeldata);
      pixeldata.SetDataElement(de);
    }
    {
      Attribute<0x002, 0x0010> at;
      at.SetFromDataSet(ds);
      TransferSyntax tstype = TransferSyntax::GetTSType(at.GetValue());
      pixeldata.SetTransferSyntax(tstype);
    }
    Internals->icons.push_back(pixeldata);
  }
}

namespace
{

struct VeproData
{
  char     ID[3];
  char     Type;
  uint16_t Width;
  uint16_t Height;
};

} // namespace

void
IconImageFilter::ExtractVeproIconImages()
{
  const DataSet &   rootds = F->GetDataSet();
  const PrivateTag  ticon1(0x55, 0x0030, "VEPRO VIF 3.0 DATA");
  const PrivateTag  ticon2(0x55, 0x0030, "VEPRO VIM 5.0 DATA");
  const ByteValue * bv = nullptr;
  if (rootds.FindDataElement(ticon1))
  {
    const DataElement & de = rootds.GetDataElement(ticon1);
    bv = de.GetByteValue();
  }
  else if (rootds.FindDataElement(ticon2))
  {
    const DataElement & de = rootds.GetDataElement(ticon2);
    bv = de.GetByteValue();
  }
  if (bv)
  {
    const char * buf = bv->GetPointer();
    size_t       len = bv->GetLength();
    VeproData    data;
    memcpy(&data, buf, sizeof(data));
    const char * raw = buf + sizeof(data);
    size_t       offset = 4;
    int          magic = memcmp(raw, "RAW\0", 4);
    if (magic != 0)
      return;
    unsigned int dims[3] = {};
    dims[0] = data.Width;
    dims[1] = data.Height;
    assert(dims[0] * dims[1] == len - sizeof(data) - offset);
    DataElement pd;
    pd.SetByteValue(raw + offset, static_cast<uint32_t>(len - sizeof(data) - offset));
    SmartPointer<IconImage> si1 = new IconImage;
    IconImage &             pixeldata = *si1;
    pixeldata.SetDataElement(pd);
    pixeldata.SetDimension(0, dims[0]);
    pixeldata.SetDimension(1, dims[1]);
    PixelFormat pf = PixelFormat::UINT8;
    pixeldata.SetPixelFormat(pf);
    pixeldata.SetPhotometricInterpretation(PhotometricInterpretation::MONOCHROME2);
    Internals->icons.push_back(pixeldata);
  }
}

} // end namespace mdcm
