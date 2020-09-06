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
#include "mdcmImageReader.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValue.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmElement.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmTransferSyntax.h"
#include "mdcmAttribute.h"
#include "mdcmImageHelper.h"
#include "mdcmPrivateTag.h"
#include "mdcmJPEGCodec.h"

namespace mdcm
{

ImageReader::ImageReader()
{
  PixelData = new Image;
}

ImageReader::~ImageReader()
{
}

const Image& ImageReader::GetImage() const
{
  return dynamic_cast<const Image&>(*PixelData);
}

Image& ImageReader::GetImage()
{
  return dynamic_cast<Image&>(*PixelData);
}

bool ImageReader::Read()
{
  return PixmapReader::Read();
}

bool ImageReader::ReadImage(MediaStorage const & ms)
{
  if(!PixmapReader::ReadImage(ms))
  {
    return false;
  }
  Image& pixeldata = GetImage();
  // Pixel Spacing
  std::vector<double> spacing = ImageHelper::GetSpacingValue(*F);
  // Only SC is allowed not to have spacing
  if(!spacing.empty())
  {
    // In MR, you can have a Z spacing, but store a 2D image
    assert(spacing.size() >= pixeldata.GetNumberOfDimensions());
    pixeldata.SetSpacing(&spacing[0]);
    if(spacing.size() > pixeldata.GetNumberOfDimensions()) // HACK
    {
      pixeldata.SetSpacing(pixeldata.GetNumberOfDimensions(), spacing[pixeldata.GetNumberOfDimensions()]);
    }
  }
  // Origin
  std::vector<double> origin = ImageHelper::GetOriginValue(*F);
  if(!origin.empty())
  {
    pixeldata.SetOrigin(&origin[0]);
    if(origin.size() > pixeldata.GetNumberOfDimensions()) // HACK
    {
      pixeldata.SetOrigin(pixeldata.GetNumberOfDimensions(), origin[pixeldata.GetNumberOfDimensions()]);
    }
  }
  std::vector<double> dircos = ImageHelper::GetDirectionCosinesValue(*F);
  if(!dircos.empty())
  {
    pixeldata.SetDirectionCosines(&dircos[0]);
  }
  std::vector<double> is = ImageHelper::GetRescaleInterceptSlopeValue(*F);
  pixeldata.SetIntercept(is[0]);
  pixeldata.SetSlope(is[1]);
  return true;
}

bool ImageReader::ReadACRNEMAImage()
{
  if(!PixmapReader::ReadACRNEMAImage())
  {
    return false;
  }
  const DataSet &ds = F->GetDataSet();
  Image& pixeldata = GetImage();
  // Pixel Spacing
  const Tag tpixelspacing(0x0028, 0x0030);
  if(ds.FindDataElement(tpixelspacing))
  {
    const DataElement& de = ds.GetDataElement(tpixelspacing);
    Attribute<0x0028,0x0030> at;
    at.SetFromDataElement(de);
    pixeldata.SetSpacing(0, at.GetValue(0));
    pixeldata.SetSpacing(1, at.GetValue(1));
  }
  // Origin
  const Tag timageposition(0x0020, 0x0030);
  if(ds.FindDataElement(timageposition))
  {
    const DataElement& de = ds.GetDataElement(timageposition);
    Attribute<0x0020,0x0030> at = {{}};
    at.SetFromDataElement(de);
    pixeldata.SetOrigin(at.GetValues());
    if(at.GetNumberOfValues() > pixeldata.GetNumberOfDimensions()) // HACK
    {
      pixeldata.SetOrigin(pixeldata.GetNumberOfDimensions(), at.GetValue(pixeldata.GetNumberOfDimensions()));
    }
  }
  const Tag timageorientation(0x0020, 0x0035);
  if(ds.FindDataElement(timageorientation))
  {
    const DataElement& de = ds.GetDataElement(timageorientation);
    Attribute<0x0020,0x0035> at = {{1,0,0,0,1,0}};
    at.SetFromDataElement(de);
    pixeldata.SetDirectionCosines(at.GetValues());
  }
  std::vector<double> is = ImageHelper::GetRescaleInterceptSlopeValue(*F);
  pixeldata.SetIntercept(is[0]);
  pixeldata.SetSlope(is[1]);
  return true;
}


} // end namespace mdcm
