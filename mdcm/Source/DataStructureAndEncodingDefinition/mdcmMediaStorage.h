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
  typedef enum
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
    TestPhotoacousticImageStorage, // FIXME
    MS_END
  } MSType;

  typedef enum
  {
    NoObject = 0, // DICOMDIR
    Video,
    Waveform,
    Audio,
    PDF,
    URI,
    Segmentation,
    ObjectEnd
  } ObjectType;

  static const char * GetMSString(MSType);
  const char *
  GetString() const;
  static MSType
  GetMSType(const char *);
  MediaStorage(MSType type = MS_END)
    : MSField(type)
  {}
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
  SetFromFile(File const & file);
  // Advanced user only (functions should be protected level)
  // Those function are lower level than SetFromFile
  bool
  SetFromDataSet(DataSet const & ds);
  bool
  SetFromHeader(FileMetaInformation const &);
  bool
  SetFromModality(DataSet const &);
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
  SetFromSourceImageSequence(DataSet const &);

private:
  bool
  SetFromDataSetOrHeader(DataSet const &, const Tag &);
  const char *
  GetFromDataSetOrHeader(DataSet const &, const Tag &, std::string &);
  const char *
  GetFromHeader(FileMetaInformation const &, std::string &);
  const char *
         GetFromDataSet(DataSet const &, std::string &);
  MSType MSField;
};

inline std::ostream &
operator<<(std::ostream & _os, const MediaStorage & ms)
{
  const char * msstring = MediaStorage::GetMSString(ms);
  _os << (msstring ? msstring : "INVALID MEDIA STORAGE");
  return _os;
}

} // end namespace mdcm

#endif // MDCMMEDIASTORAGE_H
