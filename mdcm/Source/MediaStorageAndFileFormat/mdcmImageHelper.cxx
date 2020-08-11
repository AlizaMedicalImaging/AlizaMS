/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmImageHelper.h"
#include "mdcmMediaStorage.h"
#include "mdcmFile.h"
#include "mdcmDataSet.h"
#include "mdcmDataElement.h"
#include "mdcmItem.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmGlobal.h"
#include "mdcmDictEntry.h"
#include "mdcmDicts.h"
#include "mdcmAttribute.h"
#include "mdcmImage.h"
#include "mdcmDirectionCosines.h"
#include "mdcmSegmentedPaletteColorLookupTable.h"
#include "mdcmByteValue.h"
#include "mdcmUIDGenerator.h"
#include <cmath>

namespace mdcm
{

bool ImageHelper::ForceRescaleInterceptSlope = false;
bool ImageHelper::PMSRescaleInterceptSlope = true;
bool ImageHelper::ForcePixelSpacing = false;
bool ImageHelper::CleanUnusedBits = false;

static double SetNDigits(double x, int n)
{
    const double t = pow(10.0, n);
    return floor(x*t)/t;
}

static bool GetOriginValueFromSequence(const DataSet& ds, const Tag& tfgs, std::vector<double> &ori)
{
  if(!ds.FindDataElement(tfgs)) return false;
  SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
  if(!(sqi && sqi->GetNumberOfItems() > 0)) return false;
  const Item &item = sqi->GetItem(1);
  const DataSet & subds = item.GetNestedDataSet();
  // Plane position Sequence
  const Tag tpms(0x0020,0x9113);
  if(!subds.FindDataElement(tpms)) return false;
  SmartPointer<SequenceOfItems> sqi2 = subds.GetDataElement(tpms).GetValueAsSQ();
  if(sqi2 && !sqi2->IsEmpty())
  {
    const Item &item2 = sqi2->GetItem(1);
    const DataSet & subds2 = item2.GetNestedDataSet();
    const Tag tps(0x0020,0x0032);
    if(!subds2.FindDataElement(tps)) return false;
    const DataElement &de = subds2.GetDataElement(tps);
    Attribute<0x0020,0x0032> at;
    at.SetFromDataElement(de);
    ori.push_back(at.GetValue(0));
    ori.push_back(at.GetValue(1));
    ori.push_back(at.GetValue(2));
    return true;
  }
  return false;
}

static bool GetDirectionCosinesValueFromSequence(const DataSet& ds, const Tag& tfgs, std::vector<double> &dircos)
{
  if(!ds.FindDataElement(tfgs)) return false;
  SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
  if(!(sqi && sqi->GetNumberOfItems() > 0)) return false;
  const Item &item = sqi->GetItem(1);
  const DataSet & subds = item.GetNestedDataSet();
  // Plane position Sequence
  const Tag tpms(0x0020,0x9116);
  if(!subds.FindDataElement(tpms)) return false;
  SmartPointer<SequenceOfItems> sqi2 = subds.GetDataElement(tpms).GetValueAsSQ();
  assert(sqi2 && sqi2->GetNumberOfItems());
  const Item &item2 = sqi2->GetItem(1);
  const DataSet & subds2 = item2.GetNestedDataSet();
  const Tag tps(0x0020,0x0037);
  if(!subds2.FindDataElement(tps)) return false;
  const DataElement &de = subds2.GetDataElement(tps);
  Attribute<0x0020,0x0037> at;
  at.SetFromDataElement(de);
  dircos.push_back(at.GetValue(0));
  dircos.push_back(at.GetValue(1));
  dircos.push_back(at.GetValue(2));
  dircos.push_back(at.GetValue(3));
  dircos.push_back(at.GetValue(4));
  dircos.push_back(at.GetValue(5));
  return true;
}

static bool GetInterceptSlopeValueFromSequence(const DataSet& ds, const Tag& tfgs, std::vector<double> &intslope)
{
  if(!ds.FindDataElement(tfgs)) return false;
  SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
  if(!(sqi && sqi->GetNumberOfItems() > 0)) return false;
  const Item &item = sqi->GetItem(1);
  const DataSet & subds = item.GetNestedDataSet();
  const Tag tpms(0x0028,0x9145);
  if(!subds.FindDataElement(tpms)) return false;
  SmartPointer<SequenceOfItems> sqi2 = subds.GetDataElement(tpms).GetValueAsSQ();
  assert(sqi2);
  const Item &item2 = sqi2->GetItem(1);
  const DataSet & subds2 = item2.GetNestedDataSet();
  {
    const Tag tps(0x0028,0x1052);
    if(!subds2.FindDataElement(tps)) return false;
    const DataElement &de = subds2.GetDataElement(tps);
    Attribute<0x0028,0x1052> at;
    at.SetFromDataElement(de);
    intslope.push_back(at.GetValue());
  }
  {
    const Tag tps(0x0028,0x1053);
    if(!subds2.FindDataElement(tps)) return false;
    const DataElement &de = subds2.GetDataElement(tps);
    Attribute<0x0028,0x1053> at;
    at.SetFromDataElement(de);
    intslope.push_back(at.GetValue());
  }
  assert(intslope.size() == 2);
  return true;
}

static bool ComputeZSpacingFromIPP(const DataSet &ds, double &zspacing)
{
  const Tag t1(0x5200,0x9229);
  const Tag t2(0x5200,0x9230);
  std::vector<double> cosines;
  bool b1 = GetDirectionCosinesValueFromSequence(ds, t1, cosines)
    || GetDirectionCosinesValueFromSequence(ds,t2,cosines);
  if(!b1)
  {
    cosines.resize(6);
    bool b2 = ImageHelper::GetDirectionCosinesFromDataSet(ds, cosines);
    if(b2)
    {
      mdcmWarningMacro("Image Orientation (Patient) cannot be stored here!. Continuing");
      b1 = b2;
    }
    else
    {
      mdcmErrorMacro("Image Orientation (Patient) was not found");
      cosines[0] = 1;
      cosines[1] = 0;
      cosines[2] = 0;
      cosines[3] = 0;
      cosines[4] = 1;
      cosines[5] = 0;
    }
  }
  assert(b1 && cosines.size() == 6);
  const Tag tfgs(0x5200,0x9230);
  if(!ds.FindDataElement(tfgs)) return false;
  SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
  assert(sqi);
  double normal[3];
  DirectionCosines dc(&cosines[0]);
  dc.Cross(normal);
  std::vector<double> distances;
  SequenceOfItems::SizeType nitems = sqi->GetNumberOfItems();
  std::vector<double> dircos_subds2; dircos_subds2.resize(6);
  for(SequenceOfItems::SizeType i0 = 1; i0 <= nitems; ++i0)
  {
    const Item &item = sqi->GetItem(i0);
    const DataSet & subds = item.GetNestedDataSet();
    const Tag tpms(0x0020,0x9113);
    if(!subds.FindDataElement(tpms)) return false;
    SmartPointer<SequenceOfItems> sqi2 = subds.GetDataElement(tpms).GetValueAsSQ();
    assert(sqi2);
    const Item &item2 = sqi2->GetItem(1);
    const DataSet & subds2 = item2.GetNestedDataSet();
    if(ImageHelper::GetDirectionCosinesFromDataSet(subds2, dircos_subds2))
    {
      assert(dircos_subds2 == cosines);
    }
    const Tag tps(0x0020,0x0032);
    if(!subds2.FindDataElement(tps)) return false;
    const DataElement &de = subds2.GetDataElement(tps);
    Attribute<0x0020,0x0032> ipp;
    ipp.SetFromDataElement(de);
    double dist = 0;
    for (int i = 0; i < 3; ++i) dist += normal[i]*ipp[i];
    distances.push_back(dist);
  }
  assert(distances.size() == nitems);
  double meanspacing = 0;
  double prev = distances[0];
  for(unsigned int i = 1; i < nitems; ++i)
  {
    const double current = distances[i] - prev;
    meanspacing += current;
    prev = distances[i];
  }
  bool timeseries = false;
  if(nitems > 1)
  {
    meanspacing /= (double)(nitems - 1);
    if(meanspacing == 0.0)
    {
      mdcmDebugMacro("Assuming time series for Z-spacing");
      meanspacing = 1.0;
      timeseries = true;
    }
  }
  zspacing = meanspacing;
  if(nitems > 1) assert(zspacing != 0.0);
  if(!timeseries)
  {
    const double ZTolerance = 1e-3; // FIXME
    prev = distances[0];
    for(unsigned int i = 1; i < nitems; ++i)
    {
      const double current = distances[i] - prev;
      if(fabs(current - zspacing) > ZTolerance)
      {
        mdcmErrorMacro("z-spacing error: probably non-uniform");
        return false;
      }
      prev = distances[i];
    }
  }
  return true;
}

static bool GetSpacingValueFromSequence(const DataSet& ds, const Tag& tfgs, std::vector<double> &sp)
{
  if(!ds.FindDataElement(tfgs)) return false;
  SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
  if(!(sqi && sqi->GetNumberOfItems() > 0)) return false;
  const Item &item = sqi->GetItem(1);
  const DataSet & subds = item.GetNestedDataSet();
  const Tag tpms(0x0028,0x9110);
  if(!subds.FindDataElement(tpms)) return false;
  SmartPointer<SequenceOfItems> sqi2 = subds.GetDataElement(tpms).GetValueAsSQ();
  assert(sqi2);
  const Item &item2 = sqi2->GetItem(1);
  const DataSet & subds2 = item2.GetNestedDataSet();
  const Tag tps(0x0028,0x0030);
  if(!subds2.FindDataElement(tps)) return false;
  const DataElement &de = subds2.GetDataElement(tps);
  Attribute<0x0028,0x0030> at;
  at.SetFromDataElement(de);
  sp.push_back(at.GetValue(1));
  sp.push_back(at.GetValue(0));
  double zspacing;
  if(ComputeZSpacingFromIPP(ds, zspacing))
  {
    sp.push_back(zspacing);
  }
  else
  {
    sp.push_back(1.);
  }
  return true;
}

std::vector<double> ImageHelper::GetOriginValue(File const & f)
{
  std::vector<double> ori;
  MediaStorage ms;
  ms.SetFromFile(f);
  const DataSet& ds = f.GetDataSet();
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::OphthalmicTomographyImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag t1(0x5200,0x9229);
    const Tag t2(0x5200,0x9230);
    if(GetOriginValueFromSequence(ds,t1, ori)
     || GetOriginValueFromSequence(ds, t2, ori))
    {
      assert(ori.size() == 3);
      return ori;
    }
    ori.resize(3);
    mdcmWarningMacro("Could not find Origin");
    return ori;
  }
  if(ms == MediaStorage::NuclearMedicineImageStorage)
  {
    const Tag t1(0x0054,0x0022);
    if(ds.FindDataElement(t1))
    {
      SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(t1).GetValueAsSQ();
      if(sqi && sqi->GetNumberOfItems() >= 1)
      {
        const Item &item = sqi->GetItem(1);
        const DataSet & subds = item.GetNestedDataSet();
        const Tag timagepositionpatient(0x0020, 0x0032);
        assert(subds.FindDataElement(timagepositionpatient));
        Attribute<0x0020,0x0032> at = {{0,0,0}};
        at.SetFromDataSet(subds);
        ori.resize(at.GetNumberOfValues());
        for(unsigned int i = 0; i < at.GetNumberOfValues(); ++i)
        {
          ori[i] = at.GetValue(i);
        }
        return ori;
      }
    }
    ori.resize(3);
    mdcmWarningMacro("Could not find Origin");
    return ori;
  }
  ori.resize(3);
  const Tag timagepositionpatient(0x0020, 0x0032);
  if(ms != MediaStorage::SecondaryCaptureImageStorage && ds.FindDataElement(timagepositionpatient))
  {
    const DataElement& de = ds.GetDataElement(timagepositionpatient);
    Attribute<0x0020,0x0032> at = {{0,0,0}}; // default value if empty
    at.SetFromDataElement(de);
    for(unsigned int i = 0; i < at.GetNumberOfValues(); ++i)
    {
      ori[i] = at.GetValue(i);
    }
  }
  else
  {
    ori[0] = 0;
    ori[1] = 0;
    ori[2] = 0;
  }
  assert(ori.size() == 3);
  return ori;
}

bool ImageHelper::GetDirectionCosinesFromDataSet(DataSet const & ds, std::vector<double> & dircos)
{
  // \precondition: this dataset is not a secondary capture
  const Tag timageorientationpatient(0x0020, 0x0037);
  if(ds.FindDataElement(timageorientationpatient))
  {
    const DataElement& de = ds.GetDataElement(timageorientationpatient);
    Attribute<0x0020,0x0037> at = {{1,0,0,0,1,0}};
    at.SetFromDataElement(de);
    for(unsigned int i = 0; i < at.GetNumberOfValues(); ++i)
    {
      dircos[i] = at.GetValue(i);
    }
    DirectionCosines dc(&dircos[0]);
    if(!dc.IsValid())
    {
      dc.Normalize();
      if(dc.IsValid())
      {
        mdcmWarningMacro("DirectionCosines was not normalized. Fixed");
        const double * p = dc;
        dircos = std::vector<double>(p, p + 6);
      }
      else
      {
        // PAPYRUS_CR_InvalidIOP.dcm
        mdcmWarningMacro("Could not get DirectionCosines. Will be set to unit vector.");
        return false;
      }
    }
    return true;
  }
  return false;
}

std::vector<double> ImageHelper::GetDirectionCosinesValue(File const & f)
{
  std::vector<double> dircos;
  MediaStorage ms;
  ms.SetFromFile(f);
  const DataSet& ds = f.GetDataSet();
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag t1(0x5200,0x9229);
    const Tag t2(0x5200,0x9230);
    if(GetDirectionCosinesValueFromSequence(ds,t1, dircos)
     || GetDirectionCosinesValueFromSequence(ds, t2, dircos))
    {
      assert(dircos.size() == 6);
      return dircos;
    }
    else
    {
      dircos.resize(6);
      bool b2 = ImageHelper::GetDirectionCosinesFromDataSet(ds, dircos);
      if(b2)
      {
        mdcmWarningMacro("Image Orientation (Patient) cannot be stored here!. Continuing");
      }
      else
      {
        mdcmErrorMacro("Image Orientation (Patient) was not found");
        dircos[0] = 1;
        dircos[1] = 0;
        dircos[2] = 0;
        dircos[3] = 0;
        dircos[4] = 1;
        dircos[5] = 0;
      }
      return dircos;
    }
  }
  if(ms == MediaStorage::NuclearMedicineImageStorage)
  {
    const Tag t1(0x0054,0x0022);
    if(ds.FindDataElement(t1))
    {
      SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(t1).GetValueAsSQ();
      if(sqi && sqi->GetNumberOfItems() >= 1)
      {
        const Item &item = sqi->GetItem(1);
        const DataSet & subds = item.GetNestedDataSet();

        dircos.resize(6);
        bool b2 = ImageHelper::GetDirectionCosinesFromDataSet(subds, dircos);
        if(b2)
        {
        }
        else
        {
          mdcmErrorMacro("Image Orientation (Patient) was not found");
          dircos[0] = 1;
          dircos[1] = 0;
          dircos[2] = 0;
          dircos[3] = 0;
          dircos[4] = 1;
          dircos[5] = 0;
        }
        return dircos;
      }
    }
  }
  dircos.resize(6);
  if(ms == MediaStorage::SecondaryCaptureImageStorage || !GetDirectionCosinesFromDataSet(ds, dircos))
  {
    dircos[0] = 1;
    dircos[1] = 0;
    dircos[2] = 0;
    dircos[3] = 0;
    dircos[4] = 1;
    dircos[5] = 0;
  }
  assert(dircos.size() == 6);
  return dircos;
}

void ImageHelper::SetForceRescaleInterceptSlope(bool b)
{
  ForceRescaleInterceptSlope = b;
}

bool ImageHelper::GetForceRescaleInterceptSlope()
{
  return ForceRescaleInterceptSlope;
}

void ImageHelper::SetPMSRescaleInterceptSlope(bool b)
{
  PMSRescaleInterceptSlope = b;
}

bool ImageHelper::GetPMSRescaleInterceptSlope()
{
  return PMSRescaleInterceptSlope;
}

void ImageHelper::SetForcePixelSpacing(bool b)
{
  ForcePixelSpacing = b;
}

bool ImageHelper::GetForcePixelSpacing()
{
  return ForcePixelSpacing;
}

bool ImageHelper::GetCleanUnusedBits()
{
  return CleanUnusedBits;
}

void ImageHelper::SetCleanUnusedBits(bool b)
{
  CleanUnusedBits = b;
}

bool GetRescaleInterceptSlopeValueFromDataSet(const DataSet& ds, std::vector<double> & interceptslope)
{
  Attribute<0x0028,0x1052> at1;
  bool intercept = ds.FindDataElement(at1.GetTag());
  if(intercept)
  {
    if(!ds.GetDataElement(at1.GetTag()).IsEmpty())
    {
      at1.SetFromDataElement(ds.GetDataElement(at1.GetTag()));
      interceptslope[0] = at1.GetValue();
    }
  }
  Attribute<0x0028,0x1053> at2;
  bool slope = ds.FindDataElement(at2.GetTag());
  if (slope)
  {
    if(!ds.GetDataElement(at2.GetTag()).IsEmpty())
    {
      at2.SetFromDataElement(ds.GetDataElement(at2.GetTag()));
      interceptslope[1] = at2.GetValue();
      if(interceptslope[1] == 0)
      {
        mdcmDebugMacro("Cannot have slope == 0. Defaulting to 1.0 instead");
        interceptslope[1] = 1;
      }
    }
  }
  return intercept || slope;
}


/// This function returns pixel information about an image from its dataset
/// That includes samples per pixel and bit depth (in that order)
/// Returns a PixelFormat
PixelFormat ImageHelper::GetPixelFormatValue(const File& f)
{
  PixelFormat pf;
  const DataSet& ds = f.GetDataSet();
  {
      Attribute<0x0028,0x0100> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsAllocated(at.GetValue());
  }
  {
      Attribute<0x0028,0x0101> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetBitsStored(at.GetValue());
  }
  {
      Attribute<0x0028,0x0102> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetHighBit(at.GetValue());
  }
  {
      Attribute<0x0028,0x0103> at = { 0 };
      at.SetFromDataSet(ds);
      pf.SetPixelRepresentation(at.GetValue());
  }
  {
      Attribute<0x0028,0x0002> at = { 1 };
      at.SetFromDataSet(ds);
      pf.SetSamplesPerPixel(at.GetValue());
  }
  return pf;

}

/// This function checks tags (0x0028, 0x0010) and (0x0028, 0x0011) for the
/// rows and columns of the image in pixels (as opposed to actual distances).
/// Also checks 0054, 0081 for the number of z slices for a 3D image
/// If that tag is not present, default the z dimension to 1
std::vector<unsigned int> ImageHelper::GetDimensionsValue(const File& f)
{
  DataSet const & ds = f.GetDataSet();

  MediaStorage ms;
  ms.SetFromFile(f);
  std::vector<unsigned int> theReturn(3);
  {
    Attribute<0x0028,0x0011> at = { 0 };
    at.SetFromDataSet(ds);
    theReturn[0] = at.GetValue();
  }
  {
    Attribute<0x0028,0x0010> at = { 0 };
    at.SetFromDataSet(ds);
    theReturn[1] = at.GetValue();
  }
  {
    Attribute<0x0028,0x0008> at = { 0 };
    at.SetFromDataSet(ds);
    int numberofframes = at.GetValue();
    theReturn[2] = 1;
    if(numberofframes > 1)
    {
      theReturn[2] = at.GetValue();
    }
  }
  // ACR-NEMA legacy
  {
    Attribute<0x0028,0x0005> at = { 0 };
    if(ds.FindDataElement(at.GetTag()))
    {
      const DataElement &de = ds.GetDataElement(at.GetTag());
      // SIEMENS_MAGNETOM-12-MONO2-Uncompressed.dcm picks VR::SS instead...
      if(at.GetVR().Compatible(de.GetVR()))
      {
        at.SetFromDataSet(ds);
        int imagedimensions = at.GetValue();
        if(imagedimensions == 3)
        {
          Attribute<0x0028,0x0012> at2 = { 0 };
          at2.SetFromDataSet(ds);
          theReturn[2] = at2.GetValue();
        }
      }
      else
      {
        mdcmWarningMacro("Sorry cant read attribute (wrong VR): " << at.GetTag());
      }
    }
  }
  return theReturn;
}

void ImageHelper::SetDimensionsValue(File& f, const Pixmap & img)
{
  const unsigned int *dims = img.GetDimensions();
  MediaStorage ms;
  ms.SetFromFile(f);
  DataSet& ds = f.GetDataSet();
  assert(MediaStorage::IsImage(ms));
  {
    Attribute<0x0028,0x0010> rows;
    rows.SetValue((uint16_t)dims[1]);
    ds.Replace(rows.GetAsDataElement());
    Attribute<0x0028,0x0011> columns;
    columns.SetValue((uint16_t)dims[0]);
    ds.Replace(columns.GetAsDataElement());
    Attribute<0x0028,0x0008> numframes = { 0 };
    numframes.SetValue(dims[2]);
    if(img.GetNumberOfDimensions() == 3 && dims[2] > 1)
    {
      if(ms.MediaStorage::GetModalityDimension() > 2)
        ds.Replace(numframes.GetAsDataElement());
      else
      {
        mdcmErrorMacro("MediaStorage does not allow 3rd dimension. But value is: " << dims[2]);
        mdcmAssertAlwaysMacro("Could not set third dimension");
      }
    }
    else if(img.GetNumberOfDimensions() == 2 && dims[2] == 1)
    {
      // This is a MF instances, need to set Number of Frame to 1
      if(ms.MediaStorage::GetModalityDimension() > 2)
        ds.Replace(numframes.GetAsDataElement());
    }
    else
    {
      ds.Remove(numframes.GetTag());
    }
  }
  // Cleanup
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag tfgs(0x5200,0x9230);
    if(ds.FindDataElement(tfgs))
    {
        SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
        assert(sqi);
        sqi->SetNumberOfItems(dims[2]);
    }
  }
}

std::vector<double> ImageHelper::GetRescaleInterceptSlopeValue(File const & f)
{
  std::vector<double> interceptslope;
  MediaStorage ms;
  ms.SetFromFile(f);
  const DataSet& ds = f.GetDataSet();

  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag t1(0x5200,0x9229);
    const Tag t2(0x5200,0x9230);
    if(GetInterceptSlopeValueFromSequence(ds,t1, interceptslope)
     || GetInterceptSlopeValueFromSequence(ds,t2, interceptslope))
    {
      assert(interceptslope.size() == 2);
      return interceptslope;
    }
  }

  interceptslope.resize(2);
  interceptslope[0] = 0;
  interceptslope[1] = 1;
  if(ms == MediaStorage::CTImageStorage
  || ms == MediaStorage::ComputedRadiographyImageStorage
  || ms == MediaStorage::PETImageStorage
  || ms == MediaStorage::SecondaryCaptureImageStorage
  || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
  || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
  || ForceRescaleInterceptSlope)
  {
    bool b = GetRescaleInterceptSlopeValueFromDataSet(ds, interceptslope);
    if(!b)
    {
      mdcmDebugMacro("No Modality LUT found (Rescale Intercept/Slope)");
    }
  }
  else if (ms == MediaStorage::MRImageStorage)
  {
#if 0
    const Tag trwvms(0x0040,0x9096); // Real World Value Mapping Sequence
    if(ds.FindDataElement(trwvms))
    {
      SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(trwvms).GetValueAsSQ();
      if(sqi)
      {
        const Tag trwvlutd(0x0040,0x9212); // Real World Value LUT Data
        if(ds.FindDataElement(trwvlutd))
        {
          mdcmAssertAlwaysMacro(0); // Not supported !
        }
        // dont know how to handle multiples:
        mdcmAssertAlwaysMacro(sqi->GetNumberOfItems() == 1);
        const Item &item = sqi->GetItem(1);
        const DataSet & subds = item.GetNestedDataSet();
        //const Tag trwvi(0x0040,0x9224); // Real World Value Intercept
        //const Tag trwvs(0x0040,0x9225); // Real World Value Slope
        Attribute<0x0040,0x9224> at1 = {0};
        at1.SetFromDataSet(subds);
        Attribute<0x0040,0x9225> at2 = {1};
        at2.SetFromDataSet(subds);
        interceptslope[0] = at1.GetValue();
        interceptslope[1] = at2.GetValue();
      }
    }
#else
    // See the long thread at:
    // https://groups.google.com/d/msg/comp.protocols.dicom/M4kdqcrs50Y/_TSx0EjtAQAJ
    // in particular this paper:
    // Errors in Quantitative Image Analysis due to Platform-Dependent Image Scaling
    // http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3998685/
    const PrivateTag tpriv_rescaleintercept(0x2005,0x09,"Philips MR Imaging DD 005");
    const PrivateTag tpriv_rescaleslope(0x2005,0x0a,"Philips MR Imaging DD 005");
    if(ds.FindDataElement(tpriv_rescaleintercept) && ds.FindDataElement(tpriv_rescaleslope))
    {
      // The following will work out of the box for Philips whether or not
      // "Combine MR Rescaling" was set:
      // PMS DICOM CS states that Modality LUT for MR Image Storage is to be
      // used for image processing. VOI LUT are always recomputed, so output
      // from MDCM may not look right for display (sorry!)
      const DataElement &priv_rescaleintercept = ds.GetDataElement(tpriv_rescaleintercept);
      const DataElement &priv_rescaleslope = ds.GetDataElement(tpriv_rescaleslope);
      Element<VR::DS,VM::VM1> el_ri = {{ 0 }};
      el_ri.SetFromDataElement(priv_rescaleintercept);
      Element<VR::DS,VM::VM1> el_rs = {{ 1 }};
      el_rs.SetFromDataElement(priv_rescaleslope);
      if(PMSRescaleInterceptSlope)
      {
        interceptslope[0] = el_ri.GetValue();
        interceptslope[1] = el_rs.GetValue();
        if(interceptslope[1] == 0) interceptslope[1] = 1;
        mdcmWarningMacro("PMS Modality LUT loaded for MR Image Storage: [" << interceptslope[0] << "," << interceptslope[1] << "]");
      }
    }
    else
    {
      std::vector<double> dummy(2);
      if(GetRescaleInterceptSlopeValueFromDataSet(ds, dummy))
      {
        mdcmDebugMacro("Modality LUT unused for MR Image Storage: [" << dummy[0] << "," << dummy[1] << "]");
      }
    }
#endif
  }
  else if (ms == MediaStorage::RTDoseStorage)
  {
    // TODO. FrameIncrementPointer ? (0028,0009) AT (3004,000c)
    Attribute<0x3004,0x000e> gridscaling = { 0 };
    gridscaling.SetFromDataSet(ds);
    interceptslope[0] = 0;
    interceptslope[1] = gridscaling.GetValue();
    if(interceptslope[1] == 0)
    {
      mdcmWarningMacro("Cannot have slope == 0. Defaulting to 1.0 instead");
      interceptslope[1] = 1;
    }
  }
  return interceptslope;
}

Tag ImageHelper::GetSpacingTagFromMediaStorage(MediaStorage const &ms)
{
  Tag t;
  switch(ms)
  {
  case MediaStorage::EnhancedUSVolumeStorage:
  case MediaStorage::CTImageStorage:
  case MediaStorage::MRImageStorage:
  case MediaStorage::RTDoseStorage:
  case MediaStorage::NuclearMedicineImageStorage:
  case MediaStorage::PETImageStorage:
  case MediaStorage::GeneralElectricMagneticResonanceImageStorage:
  case MediaStorage::PhilipsPrivateMRSyntheticImageStorage:
  case MediaStorage::VLPhotographicImageStorage:
  case MediaStorage::VLMicroscopicImageStorage:
  case MediaStorage::IVOCTForProcessing:
  case MediaStorage::IVOCTForPresentation:
    t = Tag(0x0028,0x0030);
    break;
  case MediaStorage::ComputedRadiographyImageStorage:
  case MediaStorage::DigitalXRayImageStorageForPresentation:
  case MediaStorage::DigitalXRayImageStorageForProcessing:
  case MediaStorage::DigitalMammographyImageStorageForPresentation:
  case MediaStorage::DigitalMammographyImageStorageForProcessing:
  case MediaStorage::DigitalIntraoralXrayImageStorageForPresentation:
  case MediaStorage::DigitalIntraoralXRayImageStorageForProcessing:
  case MediaStorage::XRayAngiographicImageStorage:
  case MediaStorage::XRayRadiofluoroscopingImageStorage:
  case MediaStorage::XRayAngiographicBiPlaneImageStorageRetired:
  case MediaStorage::EnhancedXAImageStorage:
    t = Tag(0x0018,0x1164);
    break;
  case MediaStorage::RTImageStorage:
    t = Tag(0x3002,0x0011);
    break;
  case MediaStorage::SecondaryCaptureImageStorage:
  case MediaStorage::MultiframeSingleBitSecondaryCaptureImageStorage:
  case MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage:
  case MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage:
  case MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage:
    t = Tag(0x0018,0x2010);
    break;
  case MediaStorage::GEPrivate3DModelStorage:
  case MediaStorage::Philips3D:
    t = Tag(0xffff,0xffff);
    break;
  case MediaStorage::VideoEndoscopicImageStorage:
    t = Tag(0x0018,0x1164);
    break;
  case MediaStorage::HardcopyGrayscaleImageStorage:
  case MediaStorage::HardcopyColorImageStorage:
  case MediaStorage::UltrasoundMultiFrameImageStorage:
  case MediaStorage::UltrasoundImageStorage:
  case MediaStorage::UltrasoundImageStorageRetired:
  case MediaStorage::UltrasoundMultiFrameImageStorageRetired:
    t = Tag(0x0028,0x0034);
    break;
  case MediaStorage::OphthalmicPhotography8BitImageStorage:
  case MediaStorage::OphthalmicPhotography16BitImageStorage:
    t = Tag(0x0028,0x0030);
    break;
  default:
    mdcmDebugMacro("Do not handle: " << ms);
    t = Tag(0xffff,0xffff);
    break;
  }
  // Should only override unless Modality set it already
  // basically only Secondary Capture should reach that point
  if(ForcePixelSpacing && t == Tag(0xffff,0xffff))
  {
    t = Tag(0x0028,0x0030);
  }
  return t;
}

Tag ImageHelper::GetZSpacingTagFromMediaStorage(MediaStorage const &ms)
{
  Tag t;
  switch(ms)
  {
  case MediaStorage::EnhancedUSVolumeStorage:
  case MediaStorage::MRImageStorage:
  case MediaStorage::NuclearMedicineImageStorage:
  case MediaStorage::GeneralElectricMagneticResonanceImageStorage:
    t = Tag(0x0018,0x0088);
    break;
  case MediaStorage::PETImageStorage: // ??
  case MediaStorage::CTImageStorage:
  case MediaStorage::RTImageStorage:
  case MediaStorage::ComputedRadiographyImageStorage: // ??
  case MediaStorage::DigitalXRayImageStorageForPresentation:
  case MediaStorage::DigitalXRayImageStorageForProcessing:
  case MediaStorage::DigitalMammographyImageStorageForPresentation:
  case MediaStorage::DigitalMammographyImageStorageForProcessing:
  case MediaStorage::DigitalIntraoralXrayImageStorageForPresentation:
  case MediaStorage::DigitalIntraoralXRayImageStorageForProcessing:
  case MediaStorage::XRayAngiographicImageStorage:
  case MediaStorage::XRayRadiofluoroscopingImageStorage:
  case MediaStorage::XRayAngiographicBiPlaneImageStorageRetired:
  case MediaStorage::UltrasoundImageStorage:
  case MediaStorage::UltrasoundMultiFrameImageStorage:
  case MediaStorage::UltrasoundImageStorageRetired:
  case MediaStorage::UltrasoundMultiFrameImageStorageRetired:
  case MediaStorage::SecondaryCaptureImageStorage:
  case MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage:
  case MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage:
  case MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage: //
  case MediaStorage::HardcopyGrayscaleImageStorage:
  case MediaStorage::HardcopyColorImageStorage:
    t = Tag(0xffff,0xffff);
    break;
  case MediaStorage::RTDoseStorage:
    t = Tag(0x3004,0x000c);
    break;
  case MediaStorage::GEPrivate3DModelStorage:
  case MediaStorage::Philips3D:
  case MediaStorage::VideoEndoscopicImageStorage:
    mdcmWarningMacro("FIXME");
    t = Tag(0xffff,0xffff);
    break;
  default:
    mdcmDebugMacro("Do not handle Z spacing for: " << ms);
    t = Tag(0xffff,0xffff);
    break;
  }
  if(ForcePixelSpacing && t == Tag(0xffff,0xffff))
  {
    t = Tag(0x0018,0x0088);
  }
  return t;
}

std::vector<double> ImageHelper::GetSpacingValue(File const & f)
{
  std::vector<double> sp;
  sp.reserve(3);
  MediaStorage ms;
  ms.SetFromFile(f);
  const DataSet& ds = f.GetDataSet();
  if(  ms == MediaStorage::EnhancedCTImageStorage
    || ms == MediaStorage::EnhancedMRImageStorage
    || ms == MediaStorage::EnhancedMRColorImageStorage
    || ms == MediaStorage::EnhancedPETImageStorage
    || ms == MediaStorage::OphthalmicTomographyImageStorage
    || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
    || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
    || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
    || ms == MediaStorage::XRay3DAngiographicImageStorage
    || ms == MediaStorage::XRay3DCraniofacialImageStorage
    || ms == MediaStorage::SegmentationStorage
    || ms == MediaStorage::IVOCTForProcessing
    || ms == MediaStorage::IVOCTForPresentation
    || ms == MediaStorage::BreastTomosynthesisImageStorage
    || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
    || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
    || ms == MediaStorage::ParametricMapStorage
    || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
    || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
    || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag t1(0x5200,0x9229);
    const Tag t2(0x5200,0x9230);
    if (GetSpacingValueFromSequence(ds,t1, sp) ||
        GetSpacingValueFromSequence(ds,t2, sp))
    {
      assert(sp.size() == 3);
    }
    else
    {
      sp.resize(3);
      sp[0] = 1.0;
      sp[1] = 1.0;
      sp[2] = 1.0;
      mdcmWarningMacro("Could not find Spacing");
    }
    return sp;
  }
  Tag spacingtag = GetSpacingTagFromMediaStorage(ms);
  if(spacingtag != Tag(0xffff,0xffff) && ds.FindDataElement(spacingtag) && !ds.GetDataElement(spacingtag).IsEmpty())
  {
    const DataElement& de = ds.GetDataElement(spacingtag);
    const Global &g = GlobalInstance;
    const Dicts &dicts = g.GetDicts();
    const DictEntry &entry = dicts.GetDictEntry(de.GetTag());
    const VR & vr = entry.GetVR();
    assert(vr.Compatible(de.GetVR()));
    switch(vr)
    {
    case VR::DS:
      {
        Element<VR::DS,VM::VM1_n> el;
        std::stringstream ss;
        const ByteValue *bv = de.GetByteValue();
        assert(bv);
        std::string s = std::string(bv->GetPointer(), bv->GetLength());
        ss.str(s);
        el.SetLength(entry.GetVM().GetLength() * entry.GetVR().GetSizeof());
        std::string::size_type found = s.find('\\');
        if(found != std::string::npos)
        {
          el.Read(ss);
          assert(el.GetLength() == 2);
          for(unsigned int i = 0; i < el.GetLength(); ++i)
          {
            if(el.GetValue(i))
            {
              sp.push_back(el.GetValue(i));
            }
            else
            {
              mdcmAlwaysWarnMacro("Spacing is 0, forced to 1");
              sp.push_back(1.0);
            }
          }
          std::swap(sp[0], sp[1]);
        }
        else
        {
          double singleval;
          ss >> singleval;
          if(singleval == 0.0) singleval = 1.0;
          sp.push_back(singleval);
          sp.push_back(singleval);
        }
        assert(sp.size() == (unsigned int)entry.GetVM());
      }
      break;
    case VR::IS:
      {
        Element<VR::IS,VM::VM1_n> el;
        std::stringstream ss;
        const ByteValue *bv = de.GetByteValue();
        assert(bv);
        std::string s = std::string(bv->GetPointer(), bv->GetLength());
        ss.str(s);
        el.SetLength(entry.GetVM().GetLength() * entry.GetVR().GetSizeof());
        el.Read(ss);
        for(unsigned int i = 0; i < el.GetLength(); ++i)
          sp.push_back(el.GetValue(i));
        std::swap(sp[0], sp[1]);
        assert(sp.size() == (unsigned int)entry.GetVM());
      }
      break;
    default:
      assert(0);
      break;
    }
  }
  else
  {
    sp.push_back(1.0);
    sp.push_back(1.0);
  }
  assert(sp.size() == 2);
  //
  std::vector<unsigned int> dims = ImageHelper::GetDimensionsValue(f);
  // Z
  Tag zspacingtag = ImageHelper::GetZSpacingTagFromMediaStorage(ms);
  if(zspacingtag != Tag(0xffff,0xffff) && ds.FindDataElement(zspacingtag))
  {
    const DataElement& de = ds.GetDataElement(zspacingtag);
    if(de.IsEmpty())
    {
      sp.push_back(1.0);
    }
    else
    {
      const Global &g = GlobalInstance;
      const Dicts &dicts = g.GetDicts();
      const DictEntry &entry = dicts.GetDictEntry(de.GetTag());
      const VR & vr = entry.GetVR();
      assert(de.GetVR() == vr || de.GetVR() == VR::INVALID || de.GetVR() == VR::UN);
      if(entry.GetVM() == VM::VM1)
      {
        switch(vr)
        {
        case VR::DS:
          {
            Element<VR::DS,VM::VM1_n> el;
            std::stringstream ss;
            const ByteValue *bv = de.GetByteValue();
            assert(bv);
            std::string s = std::string(bv->GetPointer(), bv->GetLength());
            ss.str(s);
            el.SetLength(entry.GetVM().GetLength() * entry.GetVR().GetSizeof());
            el.Read(ss);
            for(unsigned int i = 0; i < el.GetLength(); ++i)
            {
              const double value = el.GetValue(i);
              sp.push_back(value);
            }
          }
          break;
        default:
          assert(0);
          break;
        }
      }
      else
      {
        assert(entry.GetVM() == VM::VM2_n);
        assert(vr == VR::DS);
        Attribute<0x28,0x8> numberoframes;
        const DataElement& de1 = ds.GetDataElement(numberoframes.GetTag());
        numberoframes.SetFromDataElement(de1);
        Attribute<0x3004,0x000c> gridframeoffsetvector;
        const DataElement& de2 = ds.GetDataElement(gridframeoffsetvector.GetTag());
        gridframeoffsetvector.SetFromDataElement(de2);
        double v1 = gridframeoffsetvector[0];
        double v2 = gridframeoffsetvector[1];
        // FIXME. ? check consistency
        sp.push_back(v2 - v1);
      }
    }
  }
  else if(ds.FindDataElement(Tag(0x0028,0x0009))) // Frame Increment Pointer
  {
    const DataElement& de = ds.GetDataElement(Tag(0x0028,0x0009));
    Attribute<0x0028,0x0009,VR::AT,VM::VM1> at;
    at.SetFromDataElement(de);
    assert(ds.FindDataElement(at.GetTag()));
    if(ds.FindDataElement(at.GetValue()))
    {
      const DataElement& de2 = ds.GetDataElement(at.GetValue());
      if(at.GetValue() == Tag(0x0018,0x1063) && at.GetNumberOfValues() == 1)
      {
        Attribute<0x0018,0x1063> at2;
        at2.SetFromDataElement(de2);
        if(dims[2] > 1)
        {
          sp.push_back(at2.GetValue());
        }
        else
        {
          if(at2.GetValue() != 0.)
          {
            mdcmErrorMacro("Number of Frame should be equal to 0");
            sp.push_back(0.0);
          }
          else
          {
            sp.push_back(1.0);
          }
        }
      }
      else
      {
        mdcmWarningMacro("Don't know how to handle spacing for: " << de);
        sp.push_back(1.0);
      }
    }
    else
    {
      mdcmErrorMacro("Tag: " << at.GetTag() << " was found to point to missing"
        "Tag: " << at.GetValue() << " default to 1.0.");
      sp.push_back(1.0);
    }
  }
  else
  {
    sp.push_back(1.0);
  }
  assert(sp.size() == 3);
  assert(sp[0] != 0.);
  assert(sp[1] != 0.);
  return sp;
}

void ImageHelper::SetSpacingValue(DataSet & ds, const std::vector<double> & spacing)
{
  MediaStorage ms;
  ms.SetFromDataSet(ds);
  if(ms == MediaStorage::SecondaryCaptureImageStorage)
  {
    Tag pixelspacing(0x0028,0x0030);
    Tag imagerpixelspacing(0x0018,0x1164);
    Tag spacingbetweenslice(0x0018,0x0088);
  }
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    {
        const Tag tfgs(0x5200,0x9229);
        SmartPointer<SequenceOfItems> sqi;
        if(!ds.FindDataElement(tfgs))
      {
          sqi = new SequenceOfItems;
          DataElement de(tfgs);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi);
          de.SetVLToUndefined();
          ds.Insert(de);
      }
        sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
        sqi->SetLengthToUndefined();

        if(!sqi->GetNumberOfItems())
      {
          Item item; //(Tag(0xfffe,0xe000));
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
        Item &item1 = sqi->GetItem(1);
        DataSet &subds = item1.GetNestedDataSet();
        const Tag tpms(0x0028,0x9110);
        if(!subds.FindDataElement(tpms))
      {
          SmartPointer<SequenceOfItems> sqi2 = new SequenceOfItems;
          DataElement de(tpms);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi2);
          de.SetVLToUndefined();
          subds.Insert(de);
      }

        sqi = subds.GetDataElement(tpms).GetValueAsSQ();
        sqi->SetLengthToUndefined();

        if(!sqi->GetNumberOfItems())
      {
          Item item; //(Tag(0xfffe,0xe000));
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
        Item &item2 = sqi->GetItem(1);
        DataSet &subds2 = item2.GetNestedDataSet();

        Attribute<0x0018,0x0088> at2;
        at2.SetValue(SetNDigits(fabs(spacing[2]), 6));
        Attribute<0x0028,0x0030> at1;
        at1.SetValue(SetNDigits(spacing[1], 6), 0);
        at1.SetValue(SetNDigits(spacing[0], 6), 1);
        Attribute<0x0018,0x0050> at3;
        at3.SetValue(SetNDigits(fabs(spacing[2]), 6));
        subds2.Replace(at1.GetAsDataElement());
        subds2.Replace(at2.GetAsDataElement());
        subds2.Replace(at3.GetAsDataElement());

/////////////////////////////////////////////////////////////////////
// TODO
      if (ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage)
      {
          const Tag tMRImageFrameTypeSequence(0x0018,0x9226);
          SmartPointer<SequenceOfItems> sqMRImageFrameTypeSequence;
          if(!subds.FindDataElement(tMRImageFrameTypeSequence))
        {
          sqMRImageFrameTypeSequence = new SequenceOfItems;
          DataElement eMRImageFrameTypeSequence(tMRImageFrameTypeSequence);
          eMRImageFrameTypeSequence.SetVR(VR::SQ);
          eMRImageFrameTypeSequence.SetValue(*sqMRImageFrameTypeSequence);
          eMRImageFrameTypeSequence.SetVLToUndefined();
          subds.Insert(eMRImageFrameTypeSequence);
        }
        sqMRImageFrameTypeSequence =
          subds.GetDataElement(tMRImageFrameTypeSequence).GetValueAsSQ();
        sqMRImageFrameTypeSequence->SetLengthToUndefined();
        if(!sqMRImageFrameTypeSequence->GetNumberOfItems())
        {
          Item item;
          item.SetVLToUndefined();
          sqMRImageFrameTypeSequence->AddItem(item);
        }
        Item    & item3  = sqMRImageFrameTypeSequence->GetItem(1);
        DataSet & subds3 = item3.GetNestedDataSet();
        Attribute<0x0008,0x9007> atFrameType;
        atFrameType.SetValue("DERIVED ",   0);
        atFrameType.SetValue("PRIMARY ",   1);
        atFrameType.SetValue("VOLUME",     2);
        atFrameType.SetValue("RESAMPLED ", 3);
        subds3.Replace(atFrameType.GetAsDataElement());
        Attribute<0x0008,0x9205> atPixelPresentation;
        atPixelPresentation.SetValue("MONOCHROME");
        subds3.Replace(atPixelPresentation.GetAsDataElement());
        Attribute<0x0008,0x9206> atVolumetricProperties;
        atVolumetricProperties.SetValue("VOLUME");
        subds3.Replace(atVolumetricProperties.GetAsDataElement());
        Attribute<0x0008,0x9207> atVolumeBasedCalculationTechnique;
        atVolumeBasedCalculationTechnique.SetValue("NONE");
        subds3.Replace(atVolumeBasedCalculationTechnique.GetAsDataElement());
      }
      else if (ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage)
      {
        const Tag tCTImageFrameTypeSequence(0x0018,0x9329);
        SmartPointer<SequenceOfItems> sqCTImageFrameTypeSequence;
        if(!subds.FindDataElement(tCTImageFrameTypeSequence))
        {
          sqCTImageFrameTypeSequence = new SequenceOfItems;
          DataElement eCTImageFrameTypeSequence(tCTImageFrameTypeSequence);
          eCTImageFrameTypeSequence.SetVR(VR::SQ);
          eCTImageFrameTypeSequence.SetValue(*sqCTImageFrameTypeSequence);
          eCTImageFrameTypeSequence.SetVLToUndefined();
          subds.Insert(eCTImageFrameTypeSequence);
        }
        sqCTImageFrameTypeSequence =
          subds.GetDataElement(tCTImageFrameTypeSequence).GetValueAsSQ();
        sqCTImageFrameTypeSequence->SetLengthToUndefined();
        if(!sqCTImageFrameTypeSequence->GetNumberOfItems())
        {
          Item item;
          item.SetVLToUndefined();
          sqCTImageFrameTypeSequence->AddItem(item);
        }
        Item &item3 = sqCTImageFrameTypeSequence->GetItem(1);
        DataSet &subds3 = item3.GetNestedDataSet();
        Attribute<0x0008,0x9007> atFrameType;
        atFrameType.SetValue("DERIVED ",   0);
        atFrameType.SetValue("PRIMARY ",   1);
        atFrameType.SetValue("VOLUME",     2);
        atFrameType.SetValue("RESAMPLED ", 3);
        subds3.Replace(atFrameType.GetAsDataElement());
        Attribute<0x0008,0x9205> atPixelPresentation;
        atPixelPresentation.SetValue("MONOCHROME");
        subds3.Replace(atPixelPresentation.GetAsDataElement());
        Attribute<0x0008,0x9206> atVolumetricProperties;
        atVolumetricProperties.SetValue("VOLUME");
        subds3.Replace(atVolumetricProperties.GetAsDataElement());
        Attribute<0x0008,0x9207> atVolumeBasedCalculationTechnique;
        atVolumeBasedCalculationTechnique.SetValue("NONE");
        subds3.Replace(atVolumeBasedCalculationTechnique.GetAsDataElement());
      }
      else if (ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
      {
        const Tag tPTImageFrameTypeSequence(0x0018,0x9751);
        SmartPointer<SequenceOfItems> sqPTImageFrameTypeSequence;
        if(!subds.FindDataElement(tPTImageFrameTypeSequence))
        {
          sqPTImageFrameTypeSequence = new SequenceOfItems;
          DataElement ePTImageFrameTypeSequence(tPTImageFrameTypeSequence);
          ePTImageFrameTypeSequence.SetVR(VR::SQ);
          ePTImageFrameTypeSequence.SetValue(*sqPTImageFrameTypeSequence);
          ePTImageFrameTypeSequence.SetVLToUndefined();
          subds.Insert(ePTImageFrameTypeSequence);
        }
        sqPTImageFrameTypeSequence =
          subds.GetDataElement(tPTImageFrameTypeSequence).GetValueAsSQ();
        sqPTImageFrameTypeSequence->SetLengthToUndefined();
        if(!sqPTImageFrameTypeSequence->GetNumberOfItems())
        {
          Item item;
          item.SetVLToUndefined();
          sqPTImageFrameTypeSequence->AddItem(item);
        }
        Item &item3 = sqPTImageFrameTypeSequence->GetItem(1);
        DataSet &subds3 = item3.GetNestedDataSet();
        Attribute<0x0008,0x9007> atFrameType;
        atFrameType.SetValue("DERIVED ",   0);
        atFrameType.SetValue("PRIMARY ",   1);
        atFrameType.SetValue("VOLUME",     2);
        atFrameType.SetValue("RESAMPLED ", 3);
        subds3.Replace(atFrameType.GetAsDataElement());
        Attribute<0x0008,0x9205> atPixelPresentation;
        atPixelPresentation.SetValue("MONOCHROME");
        subds3.Replace(atPixelPresentation.GetAsDataElement());
        Attribute<0x0008,0x9206> atVolumetricProperties;
        atVolumetricProperties.SetValue("VOLUME");
        subds3.Replace(atVolumetricProperties.GetAsDataElement());
        Attribute<0x0008,0x9207> atVolumeBasedCalculationTechnique;
        atVolumeBasedCalculationTechnique.SetValue("NONE");
        subds3.Replace(atVolumeBasedCalculationTechnique.GetAsDataElement());
      }
    }
    // Cleanup per frame
    {
        const Tag tfgs(0x5200,0x9230);
        if(ds.FindDataElement(tfgs))
      {
          SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
          assert(sqi);
          SequenceOfItems::SizeType nitems = sqi->GetNumberOfItems();
          for(SequenceOfItems::SizeType i0 = 1; i0 <= nitems; ++i0)
        {
            // Get first item:
            Item &item = sqi->GetItem(i0);
            DataSet & subds = item.GetNestedDataSet();
            const Tag tpms(0x0028,0x9110);
            subds.Remove(tpms);
        }
      }
    }
    // Cleanup root (famous MR -> EMR case) 
    {
      const Tag t1(0x0018,0x0088);
      ds.Remove(t1);
      const Tag t2(0x0028,0x0030);
      ds.Remove(t2);
    }
    return;
  }
  Tag spacingtag = GetSpacingTagFromMediaStorage(ms);
  Tag zspacingtag = GetZSpacingTagFromMediaStorage(ms);
  {
    const Tag &currentspacing = spacingtag;
    if(currentspacing != Tag(0xffff,0xffff))
    {
      DataElement de(currentspacing);
      const Global &g = GlobalInstance;
      const Dicts &dicts = g.GetDicts();
      const DictEntry &entry = dicts.GetDictEntry(de.GetTag());
      const VR & vr = entry.GetVR();
      const VM & vm = entry.GetVM(); (void)vm;
      assert(de.GetVR() == vr || de.GetVR() == VR::INVALID);
      switch(vr)
      {
      case VR::DS:
        {
          Element<VR::DS,VM::VM1_n> el;
          el.SetLength(entry.GetVM().GetLength() * vr.GetSizeof());
          assert(entry.GetVM() == VM::VM2);
          for(unsigned int i = 0; i < entry.GetVM().GetLength(); ++i)
          {
            el.SetValue(spacing[i], i);
          }
          el.SetValue(spacing[1], 0);
          el.SetValue(spacing[0], 1);
          std::stringstream os;
          el.Write(os);
          de.SetVR(VR::DS);
          if(os.str().size() % 2) os << " ";
          VL::Type osStrSize = (VL::Type)os.str().size();
          de.SetByteValue(os.str().c_str(),osStrSize);
          ds.Replace(de);
        }
        break;
      case VR::IS:
        {
          Element<VR::IS,VM::VM1_n> el;
          el.SetLength(entry.GetVM().GetLength() * vr.GetSizeof());
          assert(entry.GetVM() == VM::VM2);
          for(unsigned int i = 0; i < entry.GetVM().GetLength(); ++i)
          {
            el.SetValue((int)spacing[i], i);
          }
          std::stringstream os;
          el.Write(os);
          de.SetVR(VR::IS);
          if(os.str().size() % 2) os << " ";
          VL::Type osStrSize = (VL::Type)os.str().size();
          de.SetByteValue(os.str().c_str(), osStrSize);
          ds.Replace(de);
        }
        break;
      default:
        assert(0);
      }
    }
  }
  {
    const Tag &currentspacing = zspacingtag;
    if(currentspacing != Tag(0xffff,0xffff))
    {
      DataElement de(currentspacing);
      const Global &g = GlobalInstance;
      const Dicts &dicts = g.GetDicts();
      const DictEntry &entry = dicts.GetDictEntry(de.GetTag());
      const VR & vr = entry.GetVR();
      const VM & vm = entry.GetVM(); (void)vm;
      assert(de.GetVR() == vr || de.GetVR() == VR::INVALID);
      if(entry.GetVM() == VM::VM2_n)
      {
        assert(vr == VR::DS);
        assert(de.GetTag() == Tag(0x3004,0x000c));
        Attribute<0x28,0x8> numberoframes;
        // Make we are multiframes
        if(ds.FindDataElement(numberoframes.GetTag()))
        {
          const DataElement& de1 = ds.GetDataElement(numberoframes.GetTag());
          numberoframes.SetFromDataElement(de1);
          Element<VR::DS,VM::VM2_n> el;
          el.SetLength(numberoframes.GetValue() * vr.GetSizeof());
          assert(entry.GetVM() == VM::VM2_n);
          double spacing_start = 0;
          assert(0 < numberoframes.GetValue());
          for(int i = 0; i < numberoframes.GetValue(); ++i)
          {
            el.SetValue(spacing_start, i);
            spacing_start += spacing[2];
          }
          std::stringstream os;
          el.Write(os);
          de.SetVR(VR::DS);
          if(os.str().size() % 2) os << " ";
          VL::Type osStrSize = (VL::Type)os.str().size();
          de.SetByteValue(os.str().c_str(), osStrSize);
          ds.Replace(de);
        }
      }
      else
      {
        switch(vr)
        {
        case VR::DS:
          {
            Element<VR::DS,VM::VM1_n> el;
            el.SetLength(entry.GetVM().GetLength() * vr.GetSizeof());
            assert(entry.GetVM() == VM::VM1);
            for(unsigned int i = 0; i < entry.GetVM().GetLength(); ++i)
            {
              el.SetValue(spacing[i+2], i);
            }
            std::stringstream os;
            el.Write(os);
            de.SetVR(VR::DS);
            if(os.str().size() % 2) os << " ";
            VL::Type osStrSize = (VL::Type)os.str().size();
            de.SetByteValue(os.str().c_str(), osStrSize);
            ds.Replace(de);
          }
          break;
        default:
          assert(0);
        }
      }
    }
  }
}

static void SetDataElementInSQAsItemNumber(DataSet & ds, DataElement const & de, Tag const & sqtag, unsigned int itemidx)
{
    const Tag tfgs = sqtag;
    SmartPointer<SequenceOfItems> sqi;
    if(!ds.FindDataElement(tfgs))
    {
      SmartPointer<SequenceOfItems> sqi = new SequenceOfItems;
      DataElement detmp(tfgs);
      detmp.SetVR(VR::SQ);
      detmp.SetValue(*sqi);
      detmp.SetVLToUndefined();
      ds.Insert(detmp);
    }
    sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
    sqi->SetLengthToUndefined();

    if(sqi->GetNumberOfItems() < itemidx)
    {
      Item item;
      item.SetVLToUndefined();
      sqi->AddItem(item);
    }
    Item &item1 = sqi->GetItem(itemidx);
    DataSet &subds = item1.GetNestedDataSet();
    const Tag tpms(0x0020,0x9113);
    if(!subds.FindDataElement(tpms))
    {
      SmartPointer<SequenceOfItems> sqi2 = new SequenceOfItems;
      DataElement detmp(tpms);
      detmp.SetVR(VR::SQ);
      detmp.SetValue(*sqi2);
      detmp.SetVLToUndefined();
      subds.Insert(detmp);
    }
    sqi = subds.GetDataElement(tpms).GetValueAsSQ();
    sqi->SetLengthToUndefined();

    if(!sqi->GetNumberOfItems())
    {
      Item item;
      item.SetVLToUndefined();
      sqi->AddItem(item);
    }
    Item &item2 = sqi->GetItem(1);
    DataSet &subds2 = item2.GetNestedDataSet();

    subds2.Replace(de);
}

void ImageHelper::SetOriginValue(DataSet & ds, const Image & image)
{
  const double *origin = image.GetOrigin();
  MediaStorage ms;
  ms.SetFromDataSet(ds);
  assert(MediaStorage::IsImage(ms));

  if(ms == MediaStorage::SecondaryCaptureImageStorage)
  {
    // https://sourceforge.net/p/mdcm/bugs/322/
    // default behavior is simply to pass
    return;
  }

  // FIXME hardcoded
  if(ms != MediaStorage::CTImageStorage
   && ms != MediaStorage::MRImageStorage
   && ms != MediaStorage::RTDoseStorage
   && ms != MediaStorage::PETImageStorage
   //&& ms != MediaStorage::ComputedRadiographyImageStorage
   && ms != MediaStorage::SegmentationStorage
   && ms != MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   && ms != MediaStorage::XRay3DAngiographicImageStorage
   && ms != MediaStorage::XRay3DCraniofacialImageStorage
   && ms != MediaStorage::EnhancedMRImageStorage
   && ms != MediaStorage::EnhancedMRColorImageStorage
   && ms != MediaStorage::EnhancedPETImageStorage
   && ms != MediaStorage::EnhancedCTImageStorage
   && ms != MediaStorage::IVOCTForPresentation
   && ms != MediaStorage::IVOCTForProcessing
   && ms != MediaStorage::BreastTomosynthesisImageStorage
   && ms != MediaStorage::BreastProjectionXRayImageStorageForPresentation
   && ms != MediaStorage::BreastProjectionXRayImageStorageForProcessing
   && ms != MediaStorage::ParametricMapStorage
   && ms != MediaStorage::LegacyConvertedEnhancedMRImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedCTImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    // FIXME remove the ipp tag?
    return;
  }
  if(ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    const Tag tfgs(0x5200,0x9230);
    Attribute<0x0020,0x0032> ipp = {{0,0,0}};
    double zspacing = image.GetSpacing(2);
    unsigned int dimz = image.GetDimension(2);
    const double *cosines = image.GetDirectionCosines();
    DirectionCosines dc(cosines);
    double normal[3];
    dc.Cross(normal);
    for(unsigned int i = 0; i < dimz; ++i)
    {
      double new_origin[3];
      for (int j = 0; j < 3; j++)
      { 
        // the n'th slice is n * z-spacing aloung the IOP-derived
        // z-axis
        new_origin[j] = origin[j] + normal[j] * i * zspacing;
      }
      ipp.SetValue(SetNDigits(new_origin[0], 6), 0);
      ipp.SetValue(SetNDigits(new_origin[1], 6), 1);
      ipp.SetValue(SetNDigits(new_origin[2], 6), 2);
      SetDataElementInSQAsItemNumber(ds, ipp.GetAsDataElement(), tfgs, i+1);
      // Frame Content Sequence
      if (ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage  ||
          ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage  ||
          ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage ||
          ms == MediaStorage::SegmentationStorage)
      {
        SmartPointer<SequenceOfItems> sqi =
          ds.GetDataElement(tfgs).GetValueAsSQ();
        if (!(sqi && sqi->GetNumberOfItems()>0)) continue;
        Item    & item  = sqi->GetItem(i+1);
        DataSet & subds = item.GetNestedDataSet();
        const Tag tFrameContentSequence(0x0020,0x9111);
        if(!subds.FindDataElement(tFrameContentSequence))
        {
          SmartPointer<SequenceOfItems> sqFrameContentSequence =
            new SequenceOfItems;
          DataElement eFrameContentSequence(tFrameContentSequence);
          eFrameContentSequence.SetVR(VR::SQ);
          eFrameContentSequence.SetValue(*sqFrameContentSequence);
          eFrameContentSequence.SetVLToUndefined();
          subds.Insert(eFrameContentSequence);
        }
        SmartPointer<SequenceOfItems> sqFrameContentSequence =
          subds.GetDataElement(tFrameContentSequence).GetValueAsSQ();
        sqFrameContentSequence->SetLengthToUndefined();
        if(!sqFrameContentSequence->GetNumberOfItems())
        {
          Item item;
          item.SetVLToUndefined();
          sqFrameContentSequence->AddItem(item);
        }
        Item &item3 = sqFrameContentSequence->GetItem(1);
        DataSet &subds3 = item3.GetNestedDataSet();
        Attribute<0x0020,0x9056> atStackID;
        atStackID.SetValue("1 ");
        subds3.Replace(atStackID.GetAsDataElement());
        Attribute<0x0020,0x9057> atInStackPositionNumber;
        atInStackPositionNumber.SetValue((unsigned int)(i+1));
        subds3.Replace(atInStackPositionNumber.GetAsDataElement());
        Attribute<0x0020,0x9157, VR::UL, VM::VM2> atDimensionIndexValues = {{0,0}};
        atDimensionIndexValues.SetValue(1, 0);
        atDimensionIndexValues.SetValue((unsigned int)(i+1), 1);
        subds3.Replace(atDimensionIndexValues.GetAsDataElement());
        if (ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage ||
            ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage ||
            ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
        {
          const Tag tConversionSourceAttributesSequence(0x0020,0x9172);
          if(!subds.FindDataElement(tConversionSourceAttributesSequence))
          {
            SmartPointer<SequenceOfItems> sqConversionSourceAttributesSequence =
             new SequenceOfItems;
            DataElement eConversionSourceAttributesSequence(
              tConversionSourceAttributesSequence);
            eConversionSourceAttributesSequence.SetVR(VR::SQ);
            eConversionSourceAttributesSequence.SetValue(
              *sqConversionSourceAttributesSequence);
            eConversionSourceAttributesSequence.SetVLToUndefined();
            subds.Insert(eConversionSourceAttributesSequence);
          }
          SmartPointer<SequenceOfItems> sqConversionSourceAttributesSequence =
            subds.GetDataElement(tConversionSourceAttributesSequence).GetValueAsSQ();
          sqConversionSourceAttributesSequence->SetLengthToUndefined();
          if(!sqConversionSourceAttributesSequence->GetNumberOfItems())
          {
            Item item;
            item.SetVLToUndefined();
            sqConversionSourceAttributesSequence->AddItem(item);
          }
          Item &item4 = sqConversionSourceAttributesSequence->GetItem(1);
          DataSet &subds4 = item4.GetNestedDataSet();
          Attribute<0x0008,0x1150> atReferencedSOPClassUID;
          if (ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage)
            atReferencedSOPClassUID.SetValue(
              "1.2.840.10008.5.1.4.1.1.2.2");
          else if (ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage)
            atReferencedSOPClassUID.SetValue(
              "1.2.840.10008.5.1.4.1.1.4.4");
          else if (ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
			  atReferencedSOPClassUID.SetValue(
			  "1.2.840.10008.5.1.4.1.1.128.1");
          subds4.Replace(atReferencedSOPClassUID.GetAsDataElement());
          Attribute<0x0008,0x1155> atReferencedSOPInstanceUID;
          UIDGenerator gReferencedSOPInstanceUID;
          atReferencedSOPInstanceUID.SetValue(gReferencedSOPInstanceUID.Generate());
          subds4.Replace(atReferencedSOPInstanceUID.GetAsDataElement());
      }
    }
  }
  // Sleanup the sharedgroup
  {
      const Tag tfgs0(0x5200,0x9229);
      if(ds.FindDataElement(tfgs0))
    {
        SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs0).GetValueAsSQ();
        assert(sqi);
        SequenceOfItems::SizeType nitems = sqi->GetNumberOfItems();
        for(SequenceOfItems::SizeType i0 = 1; i0 <= nitems; ++i0)
      {
          Item &item = sqi->GetItem(i0);
          DataSet & subds = item.GetNestedDataSet();
          const Tag tpms(0x0020,0x9113);
          subds.Remove(tpms);
      }
    }
  }
  // Cleanup root level
  {
    const Tag tiop(0x0020,0x0032);
    ds.Remove(tiop);
  }
  // Frame Increment Pointer
  if(  ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
    || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
    || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
    )
  {
    if(dimz > 1)
    {
      Attribute<0x0028,0x0009> fip;
      fip.SetNumberOfValues(1);
      fip.SetValue(tfgs);
      ds.Replace(fip.GetAsDataElement());
    }
  }
  return;
  }
  // Image Position Patient
  Attribute<0x0020,0x0032> ipp = {{0,0,0}};
  ipp.SetValue(origin[0], 0);
  ipp.SetValue(origin[1], 1);
  ipp.SetValue(origin[2], 2);
  ds.Replace(ipp.GetAsDataElement());
}

void ImageHelper::SetDirectionCosinesValue(DataSet & ds, const std::vector<double> & dircos)
{
  MediaStorage ms;
  ms.SetFromDataSet(ds);
  assert(MediaStorage::IsImage(ms));
  if(ms == MediaStorage::SecondaryCaptureImageStorage)
  {
    // https://sourceforge.net/p/mdcm/bugs/322/
    // default behavior is simply to pass
    return;
  }
  // FIXME Hardcoded
  if( ms != MediaStorage::CTImageStorage
   && ms != MediaStorage::MRImageStorage
   && ms != MediaStorage::RTDoseStorage
   && ms != MediaStorage::PETImageStorage
   //&& ms != MediaStorage::ComputedRadiographyImageStorage
   && ms != MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   && ms != MediaStorage::SegmentationStorage
   && ms != MediaStorage::XRay3DAngiographicImageStorage
   && ms != MediaStorage::XRay3DCraniofacialImageStorage
   && ms != MediaStorage::EnhancedMRImageStorage
   && ms != MediaStorage::EnhancedMRColorImageStorage
   && ms != MediaStorage::EnhancedPETImageStorage
   && ms != MediaStorage::EnhancedCTImageStorage
   && ms != MediaStorage::IVOCTForPresentation
   && ms != MediaStorage::IVOCTForProcessing
   && ms != MediaStorage::BreastTomosynthesisImageStorage
   && ms != MediaStorage::BreastProjectionXRayImageStorageForPresentation
   && ms != MediaStorage::BreastProjectionXRayImageStorageForProcessing
   && ms != MediaStorage::ParametricMapStorage
   && ms != MediaStorage::LegacyConvertedEnhancedMRImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedCTImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    // FIXME: ? remove the iop tag
    return;
  }
  // Image Orientation Patient
  Attribute<0x0020,0x0037> iop = {{1,0,0,0,1,0}};
  assert(dircos.size() == 6);
  DirectionCosines dc(&dircos[0]);
  if(!dc.IsValid())
  {
    mdcmWarningMacro("Direction Cosines are not valid. Using default value (1\\0\\0\\0\\1\\0)");
  }
  else
  {
    iop.SetValue(dircos[0], 0);
    iop.SetValue(dircos[1], 1);
    iop.SetValue(dircos[2], 2);
    iop.SetValue(dircos[3], 3);
    iop.SetValue(dircos[4], 4);
    iop.SetValue(dircos[5], 5);
  }
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   || ms == MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage //
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::SegmentationStorage
   || ms == MediaStorage::IVOCTForPresentation
   || ms == MediaStorage::IVOCTForProcessing
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    {
      const Tag tfgs(0x5200,0x9229);
      SmartPointer<SequenceOfItems> sqi;
      if(!ds.FindDataElement(tfgs))
      {
          sqi = new SequenceOfItems;
          DataElement de(tfgs);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi);
          de.SetVLToUndefined();
          ds.Insert(de);
      }
      sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
      sqi->SetLengthToUndefined();
      if(!sqi->GetNumberOfItems())
      {
          Item item;
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
      Item &item1 = sqi->GetItem(1);
      DataSet &subds = item1.GetNestedDataSet();
      const Tag tpms(0x0020,0x9116);
      if(!subds.FindDataElement(tpms))
      {
          SequenceOfItems *sqi2 = new SequenceOfItems;
          DataElement de(tpms);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi2);
          de.SetVLToUndefined();
          subds.Insert(de);
      }
      sqi = subds.GetDataElement(tpms).GetValueAsSQ();
      sqi->SetLengthToUndefined();
      if(!sqi->GetNumberOfItems())
      {
          Item item;
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
      Item &item2 = sqi->GetItem(1);
      DataSet &subds2 = item2.GetNestedDataSet();
      subds2.Replace(iop.GetAsDataElement());
    }
    // Cleanup per-frame
    {
      const Tag tfgs(0x5200,0x9230);
      if(ds.FindDataElement(tfgs))
      {
        SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
        assert(sqi);
        SequenceOfItems::SizeType nitems = sqi->GetNumberOfItems();
        for(SequenceOfItems::SizeType i0 = 1; i0 <= nitems; ++i0)
        {
          Item &item = sqi->GetItem(i0);
          DataSet & subds = item.GetNestedDataSet();
          const Tag tpms(0x0020,0x9116);
          subds.Remove(tpms);
        }
      }
    }
    // Cleanup root level
    {
      const Tag tiop(0x0020,0x0037);
      ds.Remove(tiop);
    }
    return;
  }
  ds.Replace(iop.GetAsDataElement());
}

void ImageHelper::SetRescaleInterceptSlopeValue(File & f, const Image & img)
{
  MediaStorage ms;
  ms.SetFromFile(f);
  assert(MediaStorage::IsImage(ms));
  DataSet &ds = f.GetDataSet();
  // FIXME Hardcoded
  if(ms != MediaStorage::CTImageStorage
   && ms != MediaStorage::ComputedRadiographyImageStorage
   && ms != MediaStorage::MRImageStorage // FIXME
   && ms != MediaStorage::PETImageStorage
   && ms != MediaStorage::RTDoseStorage
   && ms != MediaStorage::SecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage
   && ms != MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage
   && ms != MediaStorage::EnhancedMRImageStorage
   && ms != MediaStorage::EnhancedMRColorImageStorage
   && ms != MediaStorage::EnhancedCTImageStorage
   && ms != MediaStorage::EnhancedPETImageStorage
   && ms != MediaStorage::XRay3DAngiographicImageStorage
   && ms != MediaStorage::XRay3DCraniofacialImageStorage
   && ms != MediaStorage::SegmentationStorage
   && ms != MediaStorage::IVOCTForPresentation
   && ms != MediaStorage::IVOCTForProcessing
   && ms != MediaStorage::BreastTomosynthesisImageStorage
   && ms != MediaStorage::BreastProjectionXRayImageStorageForPresentation
   && ms != MediaStorage::BreastProjectionXRayImageStorageForProcessing
   && ms != MediaStorage::ParametricMapStorage
   && ms != MediaStorage::LegacyConvertedEnhancedMRImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedCTImageStorage
   && ms != MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    if(img.GetIntercept() != 0. || img.GetSlope() != 1.)
    {
      throw "Impossible";
    }
    return;
  }

  if(ms == MediaStorage::SegmentationStorage) return;
  if( ms == MediaStorage::EnhancedCTImageStorage
   || ms == MediaStorage::EnhancedMRImageStorage
   || ms == MediaStorage::EnhancedMRColorImageStorage
   || ms == MediaStorage::EnhancedPETImageStorage
   || ms == MediaStorage::XRay3DAngiographicImageStorage
   || ms == MediaStorage::XRay3DCraniofacialImageStorage
   || ms == MediaStorage::BreastTomosynthesisImageStorage
   || ms == MediaStorage::BreastProjectionXRayImageStorageForPresentation
   || ms == MediaStorage::BreastProjectionXRayImageStorageForProcessing
   || ms == MediaStorage::ParametricMapStorage
   || ms == MediaStorage::LegacyConvertedEnhancedMRImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedCTImageStorage
   || ms == MediaStorage::LegacyConvertedEnhancedPETImageStorage)
  {
    {
      const Tag tfgs(0x5200,0x9229);
      SmartPointer<SequenceOfItems> sqi;
      if(!ds.FindDataElement(tfgs))
      {
          sqi = new SequenceOfItems;
          DataElement de(tfgs);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi);
          de.SetVLToUndefined();
          ds.Insert(de);
      }
      sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
      sqi->SetLengthToUndefined();
      if(!sqi->GetNumberOfItems())
      {
          Item item; //(Tag(0xfffe,0xe000));
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
      Item &item1 = sqi->GetItem(1);
      DataSet &subds = item1.GetNestedDataSet();
      const Tag tpms(0x0028,0x9145);
      if(!subds.FindDataElement(tpms))
      {
          SequenceOfItems *sqi2 = new SequenceOfItems;
          DataElement de(tpms);
          de.SetVR(VR::SQ);
          de.SetValue(*sqi2);
          de.SetVLToUndefined();
          subds.Insert(de);
      }
      sqi = subds.GetDataElement(tpms).GetValueAsSQ();
      sqi->SetLengthToUndefined();
      if(!sqi->GetNumberOfItems())
      {
          Item item; //(Tag(0xfffe,0xe000));
          item.SetVLToUndefined();
          sqi->AddItem(item);
      }
      Item &item2 = sqi->GetItem(1);
      DataSet &subds2 = item2.GetNestedDataSet();
      Attribute<0x0028,0x1052> at1;
      at1.SetValue(img.GetIntercept());
      subds2.Replace(at1.GetAsDataElement());
      Attribute<0x0028,0x1053> at2;
      at2.SetValue(img.GetSlope());
      subds2.Replace(at2.GetAsDataElement());
      Attribute<0x0028,0x1054> at3;
      at3.SetValue("US");
      subds2.Replace(at3.GetAsDataElement());
    }
    // cleanup per-frame
    {
      const Tag tfgs(0x5200,0x9230);
      if(ds.FindDataElement(tfgs))
      {
        SmartPointer<SequenceOfItems> sqi = ds.GetDataElement(tfgs).GetValueAsSQ();
        assert(sqi);
        SequenceOfItems::SizeType nitems = sqi->GetNumberOfItems();
        for(SequenceOfItems::SizeType i0 = 1; i0 <= nitems; ++i0)
        {
          // Get first item:
          Item &item = sqi->GetItem(i0);
          DataSet & subds = item.GetNestedDataSet();
          const Tag tpms(0x0028,0x9145);
          subds.Remove(tpms);
        }
      }
    }
    // Cleanup root (famous MR -> EMR case) 
    {
      const Tag t1(0x0028,0x1052);
      ds.Remove(t1);
      const Tag t2(0x0028,0x1053);
      ds.Remove(t2);
      const Tag t3(0x0028,0x1053);
      ds.Remove(t3);
    }
    return;
  }
  if(ms == MediaStorage::RTDoseStorage)
  {
    if(img.GetIntercept() != 0)
    {
      mdcmErrorMacro("Cannot have an intercept value for RTDOSE, only Scaling allowed");
      return;
    }
    Attribute<0x3004,0x00e> at2;
    at2.SetValue(img.GetSlope());
    ds.Replace(at2.GetAsDataElement());
    Attribute<0x0028,0x0009> framePointer;
    framePointer.SetNumberOfValues(1);
    framePointer.SetValue(Tag(0x3004,0x000C));
    ds.Replace(framePointer.GetAsDataElement());
    return;
  }
  if(ms == MediaStorage::MRImageStorage)
  {
#if 0
    /*
     * http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.6.16.2.html#table_C.7.6.16-12b
     (0040,9096) SQ (Sequence with undefined length)                   # u/l,1 Real World Value Mapping Sequence
     (fffe,e000) na (Item with defined length)
     (0028,3003) LO [Grey Scale LUT]                               # 14,1 LUT Explanation
     (0040,08ea) SQ (Sequence with undefined length)               # u/l,1 Measurement Units Code Sequence
     (fffe,e000) na (Item with defined length)
     (0008,0100) SH [mm2/s ]                                   # 6,1 Code Value
     (0008,0102) SH [UCUM]                                     # 4,1 Coding Scheme Designator
     (0008,0103) SH [1.4 ]                                     # 4,1 Coding Scheme Version
     (0008,0104) LO [mm2/s ]                                   # 6,1 Code Meaning
     (fffe,e0dd)
     (0040,9210) SH [GE_GREY ]                                     # 8,1 LUT Label
     (0040,9211) US 4904                                           # 2,1 Real World Value Last Value Mapped
     (0040,9216) US 359                                            # 2,1 Real World Value First Value Mapped
     (0040,9224) FD 0                                              # 8,1 Real World Value Intercept
     (0040,9225) FD 1e-06                                          # 8,1 Real World Value Slope
     */
    if(img.GetIntercept() != 0.0 || img.GetSlope() != 1.0)
    {
      SmartPointer<SequenceOfItems> sq = new SequenceOfItems;
      Item it;
      DataSet & subds = it.GetNestedDataSet();
      Attribute<0x0040,0x9224> at1 = {0};
      at1.SetValue(img.GetIntercept());
      Attribute<0x0040,0x9225> at2 = {1};
      at2.SetValue(img.GetSlope());
      subds.Insert(at1.GetAsDataElement());
      subds.Insert(at2.GetAsDataElement());
      sq->AddItem(it);
      const Tag trwvms(0x0040,0x9096); // Real World Value Mapping Sequence
      DataElement de(trwvms);
      de.SetVR(VR::SQ);
      de.SetValue(*sq);
      ds.Replace(de);
    }
    ds.Remove(Tag(0x28,0x1052));
    ds.Remove(Tag(0x28,0x1053));
    ds.Remove(Tag(0x28,0x1054));
#else
    {
      if(ForceRescaleInterceptSlope)
      {
        mdcmDebugMacro("Forcing MR Image Storage / Modality LUT: [" << img.GetIntercept() << "," << img.GetSlope());
        Attribute<0x0028,0x1052> at1;
        at1.SetValue(img.GetIntercept());
        ds.Replace(at1.GetAsDataElement());
        Attribute<0x0028,0x1053> at2;
        at2.SetValue(img.GetSlope());
        ds.Replace(at2.GetAsDataElement());
        Attribute<0x0028,0x1054> at3; // Rescale Type
        at3.SetValue("US"); // Compatible with Enhanced MR Image Storage
        ds.Replace(at3.GetAsDataElement());
      }
    }
#endif
  }
  else
  {
    Attribute<0x0028,0x1052> at1;
    at1.SetValue(img.GetIntercept());
    ds.Replace(at1.GetAsDataElement());
    Attribute<0x0028,0x1053> at2;
    at2.SetValue(img.GetSlope());
    ds.Replace(at2.GetAsDataElement());
    Attribute<0x0028,0x1054> at3; // Rescale Type
    at3.SetValue("US"); // FIXME
    if(ms == MediaStorage::SecondaryCaptureImageStorage)
    {
      // As per 3-2009, US is the only valid enumerated value
      ds.Replace(at3.GetAsDataElement());
    }
    else if(ms == MediaStorage::PETImageStorage)
    {
      // not there anyway:
      ds.Remove(at3.GetTag());
    }
    else
    {
      // In case user decide to override the default
      ds.ReplaceEmpty(at3.GetAsDataElement());
    }
  }
}

bool ImageHelper::GetRealWorldValueMappingContent(File const & f, RealWorldValueMappingContent & ret)
{
  MediaStorage ms;
  ms.SetFromFile(f);
  const DataSet& ds = f.GetDataSet();
  if(ms == MediaStorage::MRImageStorage)
  {
    const Tag trwvms(0x0040,0x9096); // Real World Value Mapping Sequence
    if(ds.FindDataElement(trwvms))
    {
      SmartPointer<SequenceOfItems> sqi0 = ds.GetDataElement(trwvms).GetValueAsSQ();
      if(sqi0)
      {
        const Tag trwvlutd(0x0040,0x9212); // Real World Value LUT Data
        if(ds.FindDataElement(trwvlutd))
        {
          mdcmAssertAlwaysMacro(0); // Not supported !
        }
        // dont know how to handle multiples
        mdcmAssertAlwaysMacro(sqi0->GetNumberOfItems() == 1);
        const Item &item0 = sqi0->GetItem(1);
        const DataSet & subds0 = item0.GetNestedDataSet();
        //const Tag trwvi(0x0040,0x9224); // Real World Value Intercept
        //const Tag trwvs(0x0040,0x9225); // Real World Value Slope
        {
          Attribute<0x0040,0x9224> at1 = {0};
          at1.SetFromDataSet(subds0);
          Attribute<0x0040,0x9225> at2 = {1};
          at2.SetFromDataSet(subds0);
          ret.RealWorldValueIntercept = at1.GetValue();
          ret.RealWorldValueSlope = at2.GetValue();
        }
        const Tag tmucs(0x0040,0x08ea); // Measurement Units Code Sequence
        if(subds0.FindDataElement(tmucs))
        {
          SmartPointer<SequenceOfItems> sqi = subds0.GetDataElement(tmucs).GetValueAsSQ();
          if(sqi)
          {
            mdcmAssertAlwaysMacro(sqi->GetNumberOfItems() == 1);
            const Item &item = sqi->GetItem(1);
            const DataSet & subds = item.GetNestedDataSet();
            Attribute<0x0008,0x0100> at1;
            at1.SetFromDataSet(subds);
            Attribute<0x0008,0x0104> at2;
            at2.SetFromDataSet(subds);
            ret.CodeValue = at1.GetValue().Trim();
            ret.CodeMeaning = at2.GetValue().Trim();
          }
        }
      }
      return true;
    }
  }
  return false;
}

PhotometricInterpretation ImageHelper::GetPhotometricInterpretationValue(File const& f)
{
  PixelFormat pf = GetPixelFormatValue(f);
  const Tag tphotometricinterpretation(0x0028, 0x0004);
  const ByteValue *photometricinterpretation =
    ImageHelper::GetPointerFromElement(tphotometricinterpretation, f);
  PhotometricInterpretation pi = PhotometricInterpretation::UNKNOWN;
  if(photometricinterpretation)
  {
    std::string photometricinterpretation_str(
      photometricinterpretation->GetPointer(),
      photometricinterpretation->GetLength());
    pi = PhotometricInterpretation::GetPIType(photometricinterpretation_str.c_str());
  }
  else
  {
    if(pf.GetSamplesPerPixel() == 1)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to MONOCHROME2");
      pi = PhotometricInterpretation::MONOCHROME2;
    }
    else if(pf.GetSamplesPerPixel() == 3)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to RGB");
      pi = PhotometricInterpretation::RGB;
    }
    else if(pf.GetSamplesPerPixel() == 4)
    {
      mdcmWarningMacro("No PhotometricInterpretation found, default to ARGB");
      pi = PhotometricInterpretation::ARGB;
    }
  }

  bool isacrnema = false;
  DataSet ds = f.GetDataSet();
  const Tag trecognitioncode(0x0008,0x0010);
  if(ds.FindDataElement(trecognitioncode) && !ds.GetDataElement(trecognitioncode).IsEmpty())
  {
    // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
    // PHILIPS_Gyroscan-12-Jpeg_Extended_Process_2_4.dcm
    mdcmDebugMacro("Mixture of ACR NEMA and DICOM file");
    isacrnema = true;
  }
  if(!pf.GetSamplesPerPixel() || (pi.GetSamplesPerPixel() != pf.GetSamplesPerPixel()))
  {
    if(pi != PhotometricInterpretation::UNKNOWN)
    {
      pf.SetSamplesPerPixel(pi.GetSamplesPerPixel());
    }
    else if (isacrnema)
    {
      assert (pf.GetSamplesPerPixel() == 0);
      assert (pi == PhotometricInterpretation::UNKNOWN);
      pf.SetSamplesPerPixel(1);
      pi = PhotometricInterpretation::MONOCHROME2;
    }
    else
    {
      mdcmWarningMacro("Cannot recognize image type. Does not looks like ACR-NEMA and is missing both Sample Per Pixel AND PhotometricInterpretation. Please report");
    }
  }
  return pi;
}

//returns the configuration of colors in a plane, either RGB RGB RGB or RRR GGG BBB
//code is borrowed from mdcmPixmapReader::ReadImage(MediaStorage const &ms)
unsigned int ImageHelper::GetPlanarConfigurationValue(const File& f)
{
  // 4. Planar Configuration
  // D 0028|0006 [US] [Planar Configuration] [1]
  const Tag planarconfiguration = Tag(0x0028, 0x0006);
  PixelFormat pf = GetPixelFormatValue(f);
  unsigned int pc = 0;
  // FIXME: Whatif planaconfiguration is send in a grayscale image... it would be empty...
  // well hopefully :(
  DataSet const & ds = f.GetDataSet();
  if(ds.FindDataElement(planarconfiguration) && !ds.GetDataElement(planarconfiguration).IsEmpty())
  {
    const DataElement& de = ds.GetDataElement(planarconfiguration);
    Attribute<0x0028,0x0006> at = { 0 };
    at.SetFromDataElement(de);

    pc = at.GetValue();
    if(pc && pf.GetSamplesPerPixel() != 3)
    {
      mdcmDebugMacro("Cannot have PlanarConfiguration=1, when Sample Per Pixel != 3");
      pc = 0;
    }
  }
  return pc;
}

//returns the lookup table of an image file
SmartPointer<LookupTable> ImageHelper::GetLUT(File const& f)
{
  DataSet const & ds = f.GetDataSet();
  PixelFormat pf = GetPixelFormatValue(f);
  PhotometricInterpretation pi = GetPhotometricInterpretationValue(f);
  // Do the Palette Color:
  // 1. Modality LUT Sequence
  bool modlut = ds.FindDataElement(Tag(0x0028,0x3000));
  if(modlut)
  {
    mdcmWarningMacro("Modality LUT (0028,3000) are not handled. Image will not be displayed properly");
  }
  // 2. LUTData (0028,3006)
  // technically I do not need to warn about LUTData since either modality lut XOR VOI LUT need to
  // be sent to require a LUT Data...
  bool lutdata = ds.FindDataElement(Tag(0x0028,0x3006));
  if(lutdata)
  {
    mdcmWarningMacro("LUT Data (0028,3006) are not handled. Image will not be displayed properly");
  }
  // 3. VOILUTSequence (0028,3010)
  bool voilut = ds.FindDataElement(Tag(0x0028,0x3010));
  if(voilut)
  {
    mdcmWarningMacro("VOI LUT (0028,3010) are not handled. Image will not be displayed properly");
  }
  // (0028,0120) US 32767 #   2, 1 PixelPaddingValue
  bool pixelpaddingvalue = ds.FindDataElement(Tag(0x0028,0x0120));
  // PS 3.3 - 2008 / C.7.5.1.1.2 Pixel Padding Value and Pixel Padding Range Limit
  if(pixelpaddingvalue)
  {
    // Technically if Pixel Padding Value is 0 on MONOCHROME2 image, then appearance should be fine
    bool vizissue = true;
    if(pf.GetPixelRepresentation() == 0)
    {
      Element<VR::US,VM::VM1> ppv;
      if(!ds.GetDataElement(Tag(0x0028,0x0120)).IsEmpty())
      {
        ppv.SetFromDataElement(ds.GetDataElement(Tag(0x0028,0x0120)));
        if(pi == PhotometricInterpretation::MONOCHROME2 && ppv.GetValue() == 0)
        {
          vizissue = false;
        }
      }
    }
    else if(pf.GetPixelRepresentation() == 1)
    {
      mdcmDebugMacro("TODO");
    }
    // test if there is any viz issue:
    if(vizissue)
    {
      mdcmDebugMacro("Pixel Padding Value (0028,0120) is not handled. Image will not be displayed properly");
    }
  }
  SmartPointer<LookupTable> lut = new LookupTable;
  const Tag testseglut(0x0028, (0x1221 + 0));
  if(ds.FindDataElement(testseglut))
  {
    lut = new SegmentedPaletteColorLookupTable;
  }
  lut->Allocate(pf.GetBitsAllocated());
  // for each red, green, blue:
  for(int i=0; i<3; ++i)
  {
    // (0028,1101) US 0\0\16
    // (0028,1102) US 0\0\16
    // (0028,1103) US 0\0\16
    const Tag tdescriptor(0x0028, (uint16_t)(0x1101 + i));
    //const Tag tdescriptor(0x0028, 0x3002);
    Element<VR::US,VM::VM3> el_us3 = {{ 0, 0, 0}};
    // Now pass the byte array to a DICOMizer:
    el_us3.SetFromDataElement(ds[tdescriptor]); //.GetValue());
    lut->InitializeLUT(LookupTable::LookupTableType(i),
      el_us3[0], el_us3[1], el_us3[2]);
    // (0028,1201) OW
    // (0028,1202) OW
    // (0028,1203) OW
    const Tag tlut(0x0028, (uint16_t)(0x1201 + i));
    //const Tag tlut(0x0028, 0x3006);
    // Segmented LUT
    // (0028,1221) OW
    // (0028,1222) OW
    // (0028,1223) OW
    const Tag seglut(0x0028, (uint16_t)(0x1221 + i));
    if(ds.FindDataElement(tlut))
    {
      const ByteValue *lut_raw = ds.GetDataElement(tlut).GetByteValue();
      if(lut_raw)
      {
        // LookupTableType::RED == 0
        lut->SetLUT(LookupTable::LookupTableType(i),
          (const unsigned char*)lut_raw->GetPointer(), lut_raw->GetLength());
      }
      else
      {
        lut->Clear();
      }
      unsigned long check =
        (el_us3.GetValue(0) ? el_us3.GetValue(0) : 65536)
        * el_us3.GetValue(2) / 8;
      assert(!lut->Initialized() || check == lut_raw->GetLength()); (void)check;
    }
    else if(ds.FindDataElement(seglut))
    {
      const ByteValue *lut_raw = ds.GetDataElement(seglut).GetByteValue();
      if(lut_raw)
      {
        lut->SetLUT(LookupTable::LookupTableType(i),
          (const unsigned char*)lut_raw->GetPointer(), lut_raw->GetLength());
      }
      else
      {
        lut->Clear();
      }
    }
    else
    {
      assert(0);
    }
  }
  if(! lut->Initialized())
  {
    mdcmDebugMacro("LUT was uninitialized!");
  }
  return lut;
}

const ByteValue* ImageHelper::GetPointerFromElement(Tag const &tag, const File& inF)
{
  const DataSet &ds = inF.GetDataSet();
  if(ds.FindDataElement(tag))
  {
    const DataElement &de = ds.GetDataElement(tag);
    return de.GetByteValue();
  }
  return 0;
}

MediaStorage ImageHelper::ComputeMediaStorageFromModality(const char *modality,
  unsigned int dimension, PixelFormat const & pixeltype,
  PhotometricInterpretation const & pi,
  double intercept , double slope)
{
  // FIXME: Planar Configuration (0028,0006) shall not be present
  MediaStorage ms = MediaStorage::SecondaryCaptureImageStorage;
  ms.GuessFromModality(modality, dimension);
  // refine for SC family
  if(dimension != 2 &&
      (ms == MediaStorage::SecondaryCaptureImageStorage
       || ms == MediaStorage::MultiframeSingleBitSecondaryCaptureImageStorage))
  {
    if(dimension == 3 &&
      pixeltype.GetSamplesPerPixel() == 1 &&
      pi == PhotometricInterpretation::MONOCHROME2 &&
      pixeltype.GetBitsAllocated() == 8 &&
      pixeltype.GetBitsStored() == 8 &&
      pixeltype.GetHighBit() == 7 &&
      pixeltype.GetPixelRepresentation() == 0)
    {
      ms = MediaStorage::MultiframeGrayscaleByteSecondaryCaptureImageStorage;
      if(intercept != 0 || slope != 1)
      {
        // A.8.3.4 Multi-frame Grayscale Byte SC Image IOD Content Constraints
        mdcmErrorMacro("Cannot have shift/scale");
        return MediaStorage::MS_END;
      }
    }
    else if(dimension == 3 &&
      pixeltype.GetSamplesPerPixel() == 1 &&
      pi == PhotometricInterpretation::MONOCHROME2 &&
      pixeltype.GetBitsAllocated() == 1 &&
      pixeltype.GetBitsStored() == 1 &&
      pixeltype.GetHighBit() == 0 &&
      pixeltype.GetPixelRepresentation() == 0)
    {
      ms = MediaStorage::MultiframeSingleBitSecondaryCaptureImageStorage;
      // FIXME does not handle bit packing...
      if(intercept != 0 || slope != 1)
      {
        mdcmDebugMacro("Cannot have shift/scale");
        return MediaStorage::MS_END;
      }
    }
    else if(dimension == 3 &&
      pixeltype.GetSamplesPerPixel() == 1 &&
      pi == PhotometricInterpretation::MONOCHROME2 &&
      pixeltype.GetBitsAllocated() == 16 &&
      pixeltype.GetBitsStored() <= 16 && pixeltype.GetBitsStored() >= 9 &&
      pixeltype.GetHighBit() == pixeltype.GetBitsStored() - 1 &&
      (pixeltype.GetPixelRepresentation() == 0 ||
        pixeltype.GetPixelRepresentation() == 1))
    {
      ms = MediaStorage::MultiframeGrayscaleWordSecondaryCaptureImageStorage;
      // A.8.4.4 Multi-frame Grayscale Word SC Image IOD Content Constraints
      // Rescale Slope and Rescale Intercept are not constrained in this IOD to
      // any particular values. E.g., they may be used to recover floating
      // point values scaled to the integer range of the stored pixel values,
      // Rescale Slope may be less than one, e.g., a Rescale Slope of 1.0/65535
      // would allow represent floating point values from 0 to 1.0.
    }
    else if(dimension == 3 &&
      pixeltype.GetSamplesPerPixel() == 3 &&
      (  pi == PhotometricInterpretation::RGB 
      || pi == PhotometricInterpretation::YBR_RCT
      || pi == PhotometricInterpretation::YBR_ICT
      || pi == PhotometricInterpretation::YBR_FULL
      || pi == PhotometricInterpretation::YBR_FULL_422) &&
      pixeltype.GetBitsAllocated() == 8 &&
      pixeltype.GetBitsStored() == 8 &&
      pixeltype.GetHighBit() == 7 &&
      pixeltype.GetPixelRepresentation() == 0)
    {
      ms = MediaStorage::MultiframeTrueColorSecondaryCaptureImageStorage;
      if(intercept != 0 || slope != 1)
      {
        mdcmDebugMacro("Cannot have shift/scale");
        return MediaStorage::MS_END;
      }
    }
    else
    {
      mdcmDebugMacro("Cannot handle Multi Frame image in SecondaryCaptureImageStorage");
      return MediaStorage::MS_END;
    }
  }
  return ms;
}

} // end namespace mdcm
