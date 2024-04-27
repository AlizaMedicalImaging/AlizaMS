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
#ifndef MDCMMEDIASTORAGE_H
#define MDCMMEDIASTORAGE_H

#include "mdcmTransferSyntax.h"

namespace mdcm
{

class Tag;
class DataSet;
class FileMetaInformation;
class File;

/*
 * MediaStorage
 *
 */
class MDCM_EXPORT MediaStorage
{
public:
  enum MSType
  {
    MediaStorageDirectoryStorage = 0,
    ComputedRadiographyImageStorage,
    DigitalXRayImageStorageForPresentation,
    DigitalXRayImageStorageForProcessing,
    DigitalMammographyImageStorageForPresentation,
    DigitalMammographyImageStorageForProcessing,
    DigitalIntraoralXrayImageStorageForPresentation,
    DigitalIntraoralXRayImageStorageForProcessing,
    CTImageStorage,
    EnhancedCTImageStorage,
    UltrasoundImageStorageRetired,
    UltrasoundImageStorage,
    UltrasoundMultiFrameImageStorageRetired,
    UltrasoundMultiFrameImageStorage,
    MRImageStorage,
    EnhancedMRImageStorage,
    MRSpectroscopyStorage,
    NuclearMedicineImageStorageRetired,
    SecondaryCaptureImageStorage,
    MultiframeSingleBitSecondaryCaptureImageStorage,
    MultiframeGrayscaleByteSecondaryCaptureImageStorage,
    MultiframeGrayscaleWordSecondaryCaptureImageStorage,
    MultiframeTrueColorSecondaryCaptureImageStorage,
    StandaloneOverlayStorage,
    StandaloneCurveStorage,
    LeadECGWaveformStorage,
    GeneralECGWaveformStorage,
    AmbulatoryECGWaveformStorage,
    HemodynamicWaveformStorage,
    CardiacElectrophysiologyWaveformStorage,
    BasicVoiceAudioWaveformStorage,
    StandaloneModalityLUTStorage,
    StandaloneVOILUTStorage,
    GrayscaleSoftcopyPresentationStateStorageSOPClass,
    XRayAngiographicImageStorage,
    XRayRadiofluoroscopingImageStorage,
    XRayAngiographicBiPlaneImageStorageRetired,
    NuclearMedicineImageStorage,
    RawDataStorage,
    SpatialRegistrationStorage,
    SpatialFiducialsStorage,
    PETImageStorage,
    RTImageStorage,
    RTDoseStorage,
    RTStructureSetStorage,
    RTPlanStorage,
    CSANonImageStorage,
    Philips3D,
    EnhancedSR,
    BasicTextSR,
    HardcopyGrayscaleImageStorage,
    ComprehensiveSR,
    DetachedStudyManagementSOPClass,
    EncapsulatedPDFStorage,
    EncapsulatedCDAStorage,
    StudyComponentManagementSOPClass,
    DetachedVisitManagementSOPClass,
    DetachedPatientManagementSOPClass,
    VideoEndoscopicImageStorage,
    GeneralElectricMagneticResonanceImageStorage,
    GEPrivate3DModelStorage,
    ToshibaPrivateDataStorage,
    MammographyCADSR,
    KeyObjectSelectionDocument,
    HangingProtocolStorage,
    ModalityPerformedProcedureStepSOPClass,
    PhilipsPrivateMRSyntheticImageStorage,
    VLPhotographicImageStorage,
    SegmentationStorage,
    RTIonPlanStorage,
    XRay3DAngiographicImageStorage,
    EnhancedXAImageStorage,
    RTIonBeamsTreatmentRecordStorage,
    SurfaceSegmentationStorage,
    VLWholeSlideMicroscopyImageStorage,
    RTTreatmentSummaryRecordStorage,
    EnhancedUSVolumeStorage,
    XRayRadiationDoseSR,
    VLEndoscopicImageStorage,
    BreastTomosynthesisImageStorage,
    FujiPrivateCRImageStorage,
    OphthalmicPhotography8BitImageStorage,
    OphthalmicPhotography16BitImageStorage,
    OphthalmicTomographyImageStorage,
    VLMicroscopicImageStorage,
    EnhancedPETImageStorage,
    VideoPhotographicImageStorage,
    XRay3DCraniofacialImageStorage,
    IVOCTForPresentation,
    IVOCTForProcessing,
    LegacyConvertedEnhancedCTImageStorage,
    LegacyConvertedEnhancedMRImageStorage,
    LegacyConvertedEnhancedPETImageStorage,
    SurfaceScanMeshStorage,
    EnhancedMRColorImageStorage,
    ParametricMapStorage,
    BreastProjectionXRayImageStorageForPresentation,
    BreastProjectionXRayImageStorageForProcessing,
    HardcopyColorImageStorage,
    DCMTKUnknownStorage,
    PhotoacousticImageStorage,
    MS_END
  };

  enum ObjectType
  {
    NoObject = 0, // DICOMDIR
    Video,
    Waveform,
    Audio,
    PDF,
    URI,
    Segmentation,
    ObjectEnd
  };

  MediaStorage() = default;

  MediaStorage(MSType t) : MSField(t)
  {}

  static const char * GetMSString(MSType);

  const char *
  GetString() const;

  static MSType
  GetMSType(const char *);

  static bool
  IsImage(MSType ts);

  operator MSType() const { return MSField; }

  const char *
  GetModality() const;

  unsigned int
  GetModalityDimension() const;

  static unsigned int
  GetNumberOfMSType();

  static unsigned int
  GetNumberOfMSString();

  static unsigned int
  GetNumberOfModality();

  // Attempt to set the MediaStorage from a file:
  // WARNING: When no MediaStorage & Modality are found BUT a PixelData element is found
  // then MediaStorage is set to the default SecondaryCaptureImageStorage (return value is
  // false in this case)
  bool
  SetFromFile(const File & file);

  // Advanced user only (functions should be protected level)
  // Those function are lower level than SetFromFile
  bool
  SetFromDataSet(const DataSet & ds);

  bool
  SetFromHeader(const FileMetaInformation &);

  bool
  SetFromModality(const DataSet &);

  void
  GuessFromModality(const char * modality, unsigned int dimension = 2);

  friend std::ostream &
  operator<<(std::ostream &, const MediaStorage &);

  bool
  IsUndefined() const
  {
    return MSField == MS_END;
  }

protected:
  void
  SetFromSourceImageSequence(const DataSet &);

private:
  bool
  SetFromDataSetOrHeader(const DataSet &, const Tag &);

  const char *
  GetFromDataSetOrHeader(const DataSet &, const Tag &, std::string &);

  const char *
  GetFromHeader(const FileMetaInformation &, std::string &);

  const char *
  GetFromDataSet(const DataSet &, std::string &);

  MSType MSField{MS_END};
};

inline std::ostream &
operator<<(std::ostream & _os, const MediaStorage & ms)
{
  const char * msstring = MediaStorage::GetMSString(ms);
  _os << (msstring ? msstring : "Invalid media storage");
  return _os;
}

} // end namespace mdcm

#endif // MDCMMEDIASTORAGE_H
