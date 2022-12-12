// This class must be completely re-written!

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
#ifndef MDCMIMAGEHELPER_H
#define MDCMIMAGEHELPER_H

#include "mdcmTypes.h"
#include "mdcmTag.h"
#include <vector>
#include "mdcmPixelFormat.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmSmartPointer.h"
#include "mdcmLookupTable.h"

// TODO completely replace and remove

namespace mdcm
{

class MediaStorage;
class DataSet;
class File;
class Image;
class Pixmap;
class ByteValue;

struct RealWorldValueMappingContent
{
  double      RealWorldValueIntercept;
  double      RealWorldValueSlope;
  std::string CodeValue;
  std::string CodeMeaning;
};

class MDCM_EXPORT ImageHelper
{
public:
  static void
  SetForceRescaleInterceptSlope(bool);
  static bool
  GetForceRescaleInterceptSlope();
  static void
  SetPMSRescaleInterceptSlope(bool);
  static bool
  GetPMSRescaleInterceptSlope();
  static void
  SetForcePixelSpacing(bool);
  static bool
  GetForcePixelSpacing();
  static bool
  GetCleanUnusedBits();
  static void
  SetCleanUnusedBits(bool);
  static void
  SetWorkaroundCornellBug(bool);
  static bool
  GetWorkaroundCornellBug();
  static void
  SetWorkaroundPredictorBug(bool);
  static bool
  GetWorkaroundPredictorBug();
  static void
  SetJpegPreserveYBRfull(bool);
  static bool
  GetJpegPreserveYBRfull();
  static void
  SetFixFewerJpegBits(bool);
  static bool
  GetFixFewerJpegBits();
  static std::vector<unsigned int>
  GetDimensionsValue(const File &);
  static void
  SetDimensionsValue(File &, const Pixmap &);
  static PixelFormat
  GetPixelFormatValue(const File &);
  static std::vector<double>
  GetRescaleInterceptSlopeValue(File const &);
  static void
  SetRescaleInterceptSlopeValue(File &, const Image &);
  static void
  SetVOILUT(File &, const Image &);
  static bool
  GetRealWorldValueMappingContent(File const &, RealWorldValueMappingContent &);
  static std::vector<double>
  GetOriginValue(File const &);
  static void
  SetOriginValue(DataSet &, const Image &);
  static std::vector<double>
  GetDirectionCosinesValue(File const &);
  static void
  SetDirectionCosinesValue(DataSet &, const std::vector<double> &);
  static std::vector<double>
  GetSpacingValue(File const &);
  static void
  SetSpacingValue(DataSet &, const std::vector<double> &);
  static bool
  GetDirectionCosinesFromDataSet(DataSet const &, std::vector<double> &);
  static PhotometricInterpretation
  GetPhotometricInterpretationValue(File const &);
  static unsigned int
  GetPlanarConfigurationValue(const File &);
  static SmartPointer<LookupTable>
  GetLUT(File const &);
  static const ByteValue *
  GetPointerFromElement(Tag const &, File const &);
  static MediaStorage
  ComputeMediaStorageFromModality(const char *                      modality,
                                  unsigned int                      dimension = 2,
                                  PixelFormat const &               pf = PixelFormat(),
                                  PhotometricInterpretation const & pi = PhotometricInterpretation(),
                                  double                            rescaleintercept = 0,
                                  double                            rescaleslope = 1);

protected:
  static Tag
  GetSpacingTagFromMediaStorage(MediaStorage const &);
  static Tag
  GetZSpacingTagFromMediaStorage(MediaStorage const &);

private:
  static bool ForceRescaleInterceptSlope;
  static bool PMSRescaleInterceptSlope;
  static bool ForcePixelSpacing;
  static bool CleanUnusedBits;
  static bool WorkaroundCornellBug;
  static bool WorkaroundPredictorBug;
  static bool JpegPreserveYBRfull;
  static bool FixFewerJpegBits;
};

} // end namespace mdcm

#endif // MDCMIMAGEHELPER_H
