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
  double RealWorldValueIntercept;
  double RealWorldValueSlope;
  std::string CodeValue;
  std::string CodeMeaning;
};

/**
 * \brief ImageHelper (internal class, not intended for user level)
 *
 * \details
 * Helper for writing World images in DICOM. DICOM has a 'template' approach to image where
 * MR Image Storage are distinct object from Enhanced MR Image Storage. For example the
 * Pixel Spacing in one object is not at the same position (ie Tag) as in the other
 * this class is the central (read: fragile) place where all the dispatching is done from
 * a unified view of a world image (typically VTK or ITK point of view) down to the low
 * level DICOM point of view.
 */
class MDCM_EXPORT ImageHelper
{
public:
  static void SetForceRescaleInterceptSlope(bool);
  static bool GetForceRescaleInterceptSlope();
  static void SetPMSRescaleInterceptSlope(bool);
  static bool GetPMSRescaleInterceptSlope();
  static void SetForcePixelSpacing(bool);
  static bool GetForcePixelSpacing();
  static bool GetCleanUnusedBits();
  static void SetCleanUnusedBits(bool);
  static std::vector<unsigned int> GetDimensionsValue(const File &);
  static void SetDimensionsValue(File &, const Pixmap &);
  static PixelFormat GetPixelFormatValue(const File &);
  static std::vector<double> GetRescaleInterceptSlopeValue(File const &);
  static void SetRescaleInterceptSlopeValue(File &, const Image &);
  static bool GetRealWorldValueMappingContent(
    File const &,
    RealWorldValueMappingContent &);
  static std::vector<double> GetOriginValue(File const &);
  static void SetOriginValue(DataSet &, const Image &);
  static std::vector<double> GetDirectionCosinesValue(File const &);
  static void SetDirectionCosinesValue(
    DataSet &,
    const std::vector<double> &);
  static std::vector<double> GetSpacingValue(File const &);
  static void SetSpacingValue(DataSet &, const std::vector<double> &);
  static bool GetDirectionCosinesFromDataSet(
    DataSet const &,
    std::vector<double> &);
  static PhotometricInterpretation GetPhotometricInterpretationValue(
    File const &);
  static unsigned int GetPlanarConfigurationValue(const File &);
  static SmartPointer<LookupTable> GetLUT(File const &);
  static const ByteValue* GetPointerFromElement(Tag const &, File const &);
  static MediaStorage ComputeMediaStorageFromModality(
    const char * modality,
    unsigned int dimension = 2,
    PixelFormat const & pf = PixelFormat(),
    PhotometricInterpretation const & pi = PhotometricInterpretation(),
    double rescaleintercept = 0,
    double rescaleslope = 1 );

protected:
  static Tag GetSpacingTagFromMediaStorage(MediaStorage const &);
  static Tag GetZSpacingTagFromMediaStorage(MediaStorage const &);

private:
  static bool ForceRescaleInterceptSlope;
  static bool PMSRescaleInterceptSlope;
  static bool ForcePixelSpacing;
  static bool CleanUnusedBits;
};

} // end namespace mdcm

#endif // MDCMIMAGEHELPER_H
