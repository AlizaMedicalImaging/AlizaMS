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
#include "mdcmMediaStorage.h"
#include "mdcmTag.h"
#include "mdcmByteValue.h"
#include "mdcmDataSet.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmFile.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmCodeString.h"

namespace mdcm
{

static const char * MSStrings[] = {
  "1.2.840.10008.1.3.10",
  "1.2.840.10008.5.1.4.1.1.1",
  "1.2.840.10008.5.1.4.1.1.1.1",
  "1.2.840.10008.5.1.4.1.1.1.1.1",
  "1.2.840.10008.5.1.4.1.1.1.2",
  "1.2.840.10008.5.1.4.1.1.1.2.1",
  "1.2.840.10008.5.1.4.1.1.1.3",
  "1.2.840.10008.5.1.4.1.1.1.3.1",
  "1.2.840.10008.5.1.4.1.1.2",
  "1.2.840.10008.5.1.4.1.1.2.1",
  "1.2.840.10008.5.1.4.1.1.6",
  "1.2.840.10008.5.1.4.1.1.6.1",
  "1.2.840.10008.5.1.4.1.1.3",
  "1.2.840.10008.5.1.4.1.1.3.1",
  "1.2.840.10008.5.1.4.1.1.4",
  "1.2.840.10008.5.1.4.1.1.4.1",
  "1.2.840.10008.5.1.4.1.1.4.2",
  "1.2.840.10008.5.1.4.1.1.5",
  "1.2.840.10008.5.1.4.1.1.7",
  "1.2.840.10008.5.1.4.1.1.7.1",
  "1.2.840.10008.5.1.4.1.1.7.2",
  "1.2.840.10008.5.1.4.1.1.7.3",
  "1.2.840.10008.5.1.4.1.1.7.4",
  "1.2.840.10008.5.1.4.1.1.8",
  "1.2.840.10008.5.1.4.1.1.9",
  "1.2.840.10008.5.1.4.1.1.9.1.1",
  "1.2.840.10008.5.1.4.1.1.9.1.2",
  "1.2.840.10008.5.1.4.1.1.9.1.3",
  "1.2.840.10008.5.1.4.1.1.9.2.1",
  "1.2.840.10008.5.1.4.1.1.9.3.1",
  "1.2.840.10008.5.1.4.1.1.9.4.1",
  "1.2.840.10008.5.1.4.1.1.10",
  "1.2.840.10008.5.1.4.1.1.11",
  "1.2.840.10008.5.1.4.1.1.11.1",
  "1.2.840.10008.5.1.4.1.1.12.1",
  "1.2.840.10008.5.1.4.1.1.12.2",
  "1.2.840.10008.5.1.4.1.1.12.3",
  "1.2.840.10008.5.1.4.1.1.20",
  "1.2.840.10008.5.1.4.1.1.66",
  "1.2.840.10008.5.1.4.1.1.66.1",
  "1.2.840.10008.5.1.4.1.1.66.2",
  // See PETAt001_PT001.dcm
  "1.2.840.10008.5.1.4.1.1.128",
  // SYNGORTImage.dcm
  "1.2.840.10008.5.1.4.1.1.481.1",
  // eclipse_dose.dcm
  "1.2.840.10008.5.1.4.1.1.481.2",
  // exRT_Structure_Set_Storage.dcm
  "1.2.840.10008.5.1.4.1.1.481.3",
  // eclipse_plan.dcm
  "1.2.840.10008.5.1.4.1.1.481.5",
  // exCSA_Non-Image_Storage.dcm
  "1.3.12.2.1107.5.9.1",
  // 3DDCM011.dcm
  "1.2.840.113543.6.6.1.3.10002",
  // EnhancedSR
  "1.2.840.10008.5.1.4.1.1.88.22",
  // BasicTextSR:
  "1.2.840.10008.5.1.4.1.1.88.11",
  // HardcopyGrayscaleImageStorage
  "1.2.840.10008.5.1.1.29",
  // ComprehensiveSR
  "1.2.840.10008.5.1.4.1.1.88.33",
  // DetachedStudyManagementSOPClass,
  "1.2.840.10008.3.1.2.3.1",
  // EncapsulatedPDFStorage
  "1.2.840.10008.5.1.4.1.1.104.1",
  // EncapsulatedCDAStorage
  "1.2.840.10008.5.1.4.1.1.104.2",
  // StudyComponentManagementSOPClass
  "1.2.840.10008.3.1.2.3.2",
  // DetachedVisitManagementSOPClass
  "1.2.840.10008.3.1.2.2.1",
  // DetachedPatientManagementSOPClass
  "1.2.840.10008.3.1.2.1.1",
  // VideoEndoscopicImageStorage
  "1.2.840.10008.5.1.4.1.1.77.1.1.1",
  // GeneralElectricMagneticResonanceImageStorage
  "1.2.840.113619.4.2",
  // GEPrivate3DModelStorage
  "1.2.840.113619.4.26",
  // Toshiba Private Data Storage
  "1.2.392.200036.9116.7.8.1.1.1",
  // MammographyCADSR,
  "1.2.840.10008.5.1.4.1.1.88.50",
  // KeyObjectSelectionDocument
  "1.2.840.10008.5.1.4.1.1.88.59",
  // HangingProtocolStorage
  "1.2.840.10008.5.1.4.38.1",
  // Modality Performed Procedure Step SOP Class
  "1.2.840.10008.3.1.2.3.3",
  // Philips Private MR Synthetic Image Storage
  "1.3.46.670589.5.0.10",
  "1.2.840.10008.5.1.4.1.1.77.1.4",   // "VL Photographic Image Storage",
  "1.2.840.10008.5.1.4.1.1.66.4",     // Segmentation Storage
  "1.2.840.10008.5.1.4.1.1.481.8",    // RT Ion Plan Storage
  "1.2.840.10008.5.1.4.1.1.13.1.1",   // XRay3DAngiographicImageStorage,
  "1.2.840.10008.5.1.4.1.1.12.1.1",   // Enhanced XA Image Storage
  "1.2.840.10008.5.1.4.1.1.481.9",    //  RTIonBeamsTreatmentRecordStorage
  "1.2.840.10008.5.1.4.1.1.66.5",     // Surface Segmentation Storage
  "1.2.840.10008.5.1.4.1.1.77.1.6",   // VLWholeSlideMicroscopyImageStorage
  "1.2.840.10008.5.1.4.1.1.481.7",    // RTTreatmentSummaryRecordStorage
  "1.2.840.10008.5.1.4.1.1.6.2",      // EnhancedUSVolumeStorage
  "1.2.840.10008.5.1.4.1.1.88.67",    // XRayRadiationDoseSR
  "1.2.840.10008.5.1.4.1.1.77.1.1",   // VLEndoscopicImageStorage
  "1.2.840.10008.5.1.4.1.1.13.1.3",   // BreastTomosynthesisImageStorage
  "1.2.392.200036.9125.1.1.2",        // FujiPrivateCRImageStorage
  "1.2.840.10008.5.1.4.1.1.77.1.5.1", // OphthalmicPhotography8BitImageStorage
  "1.2.840.10008.5.1.4.1.1.77.1.5.2", // OphthalmicPhotography16BitImageStorage
  "1.2.840.10008.5.1.4.1.1.77.1.5.4", // OphthalmicTomographyImageStorage
  "1.2.840.10008.5.1.4.1.1.77.1.2",   // VL Microscopic Image Storage
  "1.2.840.10008.5.1.4.1.1.130",      // Enhanced PET Image Storage
  "1.2.840.10008.5.1.4.1.1.77.1.4.1", // Video Photographic Image Storage
  "1.2.840.10008.5.1.4.1.1.13.1.2",   // XRay3DCraniofacialImageStorage
  "1.2.840.10008.5.1.4.1.1.14.1",     // IVOCTForPresentation,
  "1.2.840.10008.5.1.4.1.1.14.2",     // IVCOTForProcessing,
  "1.2.840.10008.5.1.4.1.1.2.2",      // Legacy Converted Enhanced CT Image Storage
  "1.2.840.10008.5.1.4.1.1.4.4",      // Legacy Converted Enhanced MR Image Storage
  "1.2.840.10008.5.1.4.1.1.128.1",    // Legacy Converted Enhanced PET Image Storage
  "1.2.840.10008.5.1.4.1.1.68.1",     // Surface Scan Mesh Storage
  "1.2.840.10008.5.1.4.1.1.4.3",      // Enhanced MR Color Image Storage
  "1.2.840.10008.5.1.4.1.1.30",       // Parametric Map Storage
  "1.2.840.10008.5.1.4.1.1.13.1.4",   // Breast Projection X-Ray Image Storage - For Presentation
  "1.2.840.10008.5.1.4.1.1.13.1.5",   // Breast Projection X-Ray Image Storage - For Processing
  "1.2.840.10008.5.1.1.30",           // HardcopyColorImageStorage
  "1.2.276.0.7230010.3.1.0.1",        // DCMTK Unknown, wrong, try to handle like secondary capture
  NULL
};

MediaStorage::MSType
MediaStorage::GetMSType(const char * str)
{
  if (!str)
    return MS_END;

  for (unsigned int i = 0; MSStrings[i] != NULL; ++i)
  {
    if (strcmp(str, MSStrings[i]) == 0)
    {
      return static_cast<MSType>(i);
    }
  }
  // We did not find anything, that's pretty bad, let's hope that
  // the toolkit which wrote the image is buggy and tolerate space padded binary
  // string
  CodeString  codestring = str;
  std::string cs = codestring.GetAsString();
  for (unsigned int i = 0; MSStrings[i] != NULL; ++i)
  {
    if (strcmp(cs.c_str(), MSStrings[i]) == 0)
    {
      return static_cast<MSType>(i);
    }
  }
  return MS_END;
}

const char *
MediaStorage::GetMSString(MSType ms)
{
  assert(ms <= MS_END);
  return MSStrings[static_cast<unsigned int>(ms)];
}

const char *
MediaStorage::GetString() const
{
  return GetMSString(MSField);
}

// clang-format off
bool
MediaStorage::IsImage(MSType ms)
{
// FIXME
  if (ms == MS_END ||
      ms == BasicVoiceAudioWaveformStorage ||
      ms == CSANonImageStorage ||
      ms == HemodynamicWaveformStorage ||
      ms == MediaStorageDirectoryStorage ||
      ms == RTPlanStorage ||
      ms == GrayscaleSoftcopyPresentationStateStorageSOPClass ||
      ms == CardiacElectrophysiologyWaveformStorage ||
      ms == ToshibaPrivateDataStorage ||
      ms == EnhancedSR ||
      ms == BasicTextSR ||
      ms == ComprehensiveSR ||
      ms == StudyComponentManagementSOPClass ||
      ms == DetachedVisitManagementSOPClass ||
      ms == DetachedStudyManagementSOPClass ||
      ms == EncapsulatedPDFStorage ||
      ms == EncapsulatedCDAStorage ||
      ms == XRayRadiationDoseSR ||
      ms == KeyObjectSelectionDocument ||
      ms == HangingProtocolStorage ||
      ms == MRSpectroscopyStorage ||
      ms == ModalityPerformedProcedureStepSOPClass ||
      ms == RawDataStorage ||
      ms == RTIonPlanStorage ||
      ms == LeadECGWaveformStorage ||
      ms == GeneralECGWaveformStorage ||
      ms == RTIonBeamsTreatmentRecordStorage ||
      ms == RTStructureSetStorage ||
      ms == MammographyCADSR ||
      ms == SurfaceSegmentationStorage ||
      ms == SurfaceScanMeshStorage)
  {
    return false;
  }
  return true;
}
// clang-format on

struct MSModalityType
{
  const char *        Modality;
  const unsigned char Dimension;
  const bool          Retired;
};

static const MSModalityType MSModalityTypes[] = {
  { "00", 0, false },       // MediaStorageDirectoryStorage,
  { "CR", 2, false },       // ComputedRadiographyImageStorage,
  { "DX", 2, false },       // DigitalXRayImageStorageForPresentation,
  { "DX", 2, false },       // DigitalXRayImageStorageForProcessing,
  { "  ", 2, false },       // DigitalMammographyImageStorageForPresentation,
  { "MG", 2, false },       // DigitalMammographyImageStorageForProcessing,
  { "  ", 2, false },       // DigitalIntraoralXrayImageStorageForPresentation,
  { "  ", 2, false },       // DigitalIntraoralXRayImageStorageForProcessing,
  { "CT", 2, false },       // CTImageStorage,
  { "CT", 3, false },       // EnhancedCTImageStorage,
  { "US", 2, true },        // UltrasoundImageStorageRetired,
  { "US", 2, false },       // UltrasoundImageStorage,
  { "US", 3, true },        // UltrasoundMultiFrameImageStorageRetired,
  { "US", 3, false },       // UltrasoundMultiFrameImageStorage,
  { "MR", 2, false },       // MRImageStorage,
  { "MR", 3, false },       // EnhancedMRImageStorage,
  { "MR", 2, false },       // MRSpectroscopyStorage,
  { "  ", 2, true },        // NuclearMedicineImageStorageRetired,
  { "OT", 2, false },       // SecondaryCaptureImageStorage,
  { "OT", 3, false },       // MultiframeSingleBitSecondaryCaptureImageStorage,
  { "OT", 3, false },       // MultiframeGrayscaleByteSecondaryCaptureImageStorage,
  { "OT", 3, false },       // MultiframeGrayscaleWordSecondaryCaptureImageStorage,
  { "OT", 3, false },       // MultiframeTrueColorSecondaryCaptureImageStorage,
  { "  ", 2, false },       // StandaloneOverlayStorage,
  { "  ", 2, false },       // StandaloneCurveStorage,
  { "  ", 2, false },       // LeadECGWaveformStorage, // 12-
  { "  ", 2, false },       // GeneralECGWaveformStorage,
  { "  ", 2, false },       // AmbulatoryECGWaveformStorage,
  { "  ", 2, false },       // HemodynamicWaveformStorage,
  { "  ", 2, false },       // CardiacElectrophysiologyWaveformStorage,
  { "  ", 2, false },       // BasicVoiceAudioWaveformStorage,
  { "  ", 2, false },       // StandaloneModalityLUTStorage,
  { "  ", 2, false },       // StandaloneVOILUTStorage,
  { "  ", 2, false },       // GrayscaleSoftcopyPresentationStateStorageSOPClass,
  { "XA", 3, false },       // XRayAngiographicImageStorage,
  { "RF", 2, false },       // XRayRadiofluoroscopingImageStorage,
  { "  ", 2, false },       // XRayAngiographicBiPlaneImageStorageRetired,
  { "NM", 3, false },       // NuclearMedicineImageStorage,
  { "  ", 2, false },       // RawDataStorage,
  { "  ", 2, false },       // SpatialRegistrationStorage,
  { "  ", 2, false },       // SpatialFiducialsStorage,
  { "PT", 2, false },       // PETImageStorage,
  { "RTIMAGE ", 2, false }, // RTImageStorage, // FIXME
  { "RTDOSE", 3, false },   // RTDoseStorage,
  { "  ", 2, false },       // RTStructureSetStorage,
  { "  ", 2, false },       // RTPlanStorage,
  { "  ", 2, false },       // CSANonImageStorage,
  { "  ", 2, false },       // Philips3D,
  { "  ", 2, false },       // EnhancedSR
  { "  ", 2, false },       // BasicTextSR
  { "  ", 2, true },        // HardcopyGrayscaleImageStorage
  { "  ", 2, false },       // ComprehensiveSR
  { "  ", 2, false },       // DetachedStudyManagementSOPClass
  { "  ", 2, false },       // EncapsulatedPDFStorage
  { "  ", 2, false },       // EncapsulatedCDAStorage
  { "  ", 2, false },       // StudyComponentManagementSOPClass
  { "  ", 2, false },       // DetachedVisitManagementSOPClass
  { "  ", 2, false },       // DetachedPatientManagementSOPClass
  { "ES", 3, false },       // VideoEndoscopicImageStorage
  { "  ", 2, false },       // GeneralElectricMagneticResonanceImageStorage
  { "  ", 2, false },       // GEPrivate3DModelStorage
  { "  ", 2, false },       // ToshibaPrivateDataStorage
  { "  ", 2, false },       // MammographyCADSR
  { "  ", 2, false },       // KeyObjectSelectionDocument
  { "  ", 2, false },       // HangingProtocolStorage
  { "  ", 2, false },       // ModalityPerformedProcedureStepSOPClass
  { "  ", 2, false },       // PhilipsPrivateMRSyntheticImageStorage
  { "XC", 2, false },       // VLPhotographicImageStorage
  { "SEG ", 3, false },     // Segmentation Storage
  { "  ", 2, false },       // RT Ion Plan Storage
  { "XA", 3, false },       // XRay3DAngiographicImageStorage,
  { "XA", 3, false },       // Enhanced XA Image Storage
  { "  ", 2, false },       // RTIonBeamsTreatmentRecordStorage
  { "SEG", 3, false },      // Surface Segmentation Storage
  { "SM", 3, false },       // VLWholeSlideMicroscopyImageStorage
  { "RTRECORD", 2, false }, // RTTreatmentSummaryRecordStorage
  { "US", 3, false },       // EnhancedUSVolumeStorage
  { "  ", 2, false },       // XRayRadiationDoseSR
  { "ES", 2, false },       // VLEndoscopicImageStorage
  { "MG", 3, false },       // BreastTomosynthesisImageStorage
  { "CR", 2, false },       // FujiPrivateCRImageStorage
  { "OP", 2, false },       // OphthalmicPhotography8BitImageStorage
  { "OP", 2, false },       // OphthalmicPhotography16BitImageStorage
  { "OPT", 3, false },      // OphthalmicTomographyImageStorage
  { "GM", 3, false },       // VLMicroscopicImageStorage
  { "PT", 3, false },       // PETImageStorage,
  { "XC", 3, false },       // VideoPhotographicImageStorage
  { "DX", 3, false },       // XRay3DCraniofacialImageStorage
  { "IVOCT", 3, false },    // IVOCTForPresentation,
  { "IVOCT", 3, false },    // IVCOTForProcessing,
  { "CT", 3, false },       // Legacy Converted Enhanced CT Image Storage
  { "MR", 3, false },       // Legacy Converted Enhanced MR Image Storage
  { "PT", 3, false },       // Legacy Converted Enhanced PET Image Storage
  { "OT", 3, false },       // Surface Segmentation Storage
  { "MR", 3, false },       // EnhancedMRColorImageStorage,
  { "OT", 3, false },       // ParametricMapImageStorage,
  { "MG", 3, false },       // BreastProjectionXRayImageStorageForPresentation
  { "MG", 3, false },       // BreastProjectionXRayImageStorageForProcessing
  { "  ", 2, false },       // HardcopyColorImageStorage
  { "  ", 2, false },       // DCMTKUnknownStorage
  { NULL, 0, false }        // MS_END
};

unsigned int
MediaStorage::GetNumberOfMSType()
{
  const unsigned int n = MS_END;
  assert(n > 0);
  return n;
}

unsigned int
MediaStorage::GetNumberOfMSString()
{
  const unsigned int n = sizeof(MSStrings) / sizeof(*MSStrings);
  assert(n > 0);
  return n - 1;
}

unsigned int
MediaStorage::GetNumberOfModality()
{
  const unsigned int n = sizeof(MSModalityTypes) / sizeof(*MSModalityTypes);
  assert(n > 0);
  return n - 1;
}

const char *
MediaStorage::GetModality() const
{
  if (!MSModalityTypes[MSField].Modality)
    return NULL;
  return MSModalityTypes[MSField].Modality;
}

unsigned int
MediaStorage::GetModalityDimension() const
{
  if (!MSModalityTypes[MSField].Modality)
    return 0;
  return MSModalityTypes[MSField].Dimension;
}

void
MediaStorage::GuessFromModality(const char * modality, unsigned int dim)
{
  // no default value is set, it is up to the user to decide initial value
  if (!modality || !dim)
    return;
  int i = 0;
  while (MSModalityTypes[i].Modality && (strcmp(modality, MSModalityTypes[i].Modality) != 0 ||
                                         MSModalityTypes[i].Retired || MSModalityTypes[i].Dimension < dim))
  {
    ++i;
  }
  if (MSModalityTypes[i].Modality)
  {
    MSField = static_cast<MSType>(i);
  }
}

const char *
MediaStorage::GetFromDataSetOrHeader(DataSet const & ds, const Tag & tag, std::string & buf)
{
  if (ds.FindDataElement(tag))
  {
    const ByteValue * sopclassuid = ds.GetDataElement(tag).GetByteValue();
    if (!sopclassuid || !sopclassuid->GetPointer())
      return NULL;
    std::string sopclassuid_str(sopclassuid->GetPointer(), sopclassuid->GetLength());
    if (sopclassuid_str.find(' ') != std::string::npos)
    {
      mdcmWarningMacro("UI contains a space character discarding");
      std::string::size_type pos = sopclassuid_str.find_last_of(' ');
      sopclassuid_str = sopclassuid_str.substr(0, pos);
    }
    buf = sopclassuid_str.c_str();
    return buf.c_str();
  }
  return NULL;
}

bool
MediaStorage::SetFromDataSetOrHeader(DataSet const & ds, const Tag & tag)
{
  std::string  buf;
  const char * ms_str = GetFromDataSetOrHeader(ds, tag, buf);
  if (ms_str)
  {
    MediaStorage ms = MediaStorage::GetMSType(ms_str);
    MSField = ms;
    if (ms == MS_END)
    {
      mdcmWarningMacro("Does not know " << ms_str);
    }
    return true;
  }
  return false;
}

const char *
MediaStorage::GetFromHeader(FileMetaInformation const & fmi, std::string & buf)
{
  const Tag tmediastoragesopclassuid(0x0002, 0x0002);
  return GetFromDataSetOrHeader(fmi, tmediastoragesopclassuid, buf);
}

bool
MediaStorage::SetFromHeader(FileMetaInformation const & fmi)
{
  const Tag tmediastoragesopclassuid(0x0002, 0x0002);
  return SetFromDataSetOrHeader(fmi, tmediastoragesopclassuid);
}

const char *
MediaStorage::GetFromDataSet(DataSet const & ds, std::string & buf)
{
  const Tag tsopclassuid(0x0008, 0x0016);
  return GetFromDataSetOrHeader(ds, tsopclassuid, buf);
}


bool
MediaStorage::SetFromDataSet(DataSet const & ds)
{
  const Tag tsopclassuid(0x0008, 0x0016);
  return SetFromDataSetOrHeader(ds, tsopclassuid);
}

void
MediaStorage::SetFromSourceImageSequence(DataSet const & ds)
{
  const Tag sourceImageSequenceTag(0x0008, 0x2112);
  if (ds.FindDataElement(sourceImageSequenceTag))
  {
    const DataElement &           sourceImageSequencesq = ds.GetDataElement(sourceImageSequenceTag);
    SmartPointer<SequenceOfItems> sq = sourceImageSequencesq.GetValueAsSQ();
    if (!(sq && sq->GetNumberOfItems() > 0))
      return;
    SequenceOfItems::ConstIterator it = sq->Begin();
    const DataSet &                subds = it->GetNestedDataSet();
    const Tag                      referencedSOPClassUIDTag(0x0008, 0x1150);
    if (subds.FindDataElement(referencedSOPClassUIDTag))
    {
      const DataElement & de = subds.GetDataElement(referencedSOPClassUIDTag);
      const ByteValue *   sopclassuid = de.GetByteValue();
      if (sopclassuid)
      {
        std::string sopclassuid_str(sopclassuid->GetPointer(), sopclassuid->GetLength());
        if (sopclassuid_str.find(' ') != std::string::npos)
        {
          mdcmWarningMacro("UI contains a space character discarding");
          std::string::size_type pos = sopclassuid_str.find_last_of(' ');
          sopclassuid_str = sopclassuid_str.substr(0, pos);
        }
        MediaStorage ms = MediaStorage::GetMSType(sopclassuid_str.c_str());
        assert(ms != MS_END);
        MSField = ms;
      }
    }
  }
}

bool
MediaStorage::SetFromModality(DataSet const & ds)
{
  // Attempt to recover from the modality (0008,0060)
  if (ds.FindDataElement(Tag(0x0008, 0x0060)))
  {
    // mdcm-CR-DCMTK-16-NonSamplePerPix.dcm
    const ByteValue * bv = ds.GetDataElement(Tag(0x0008, 0x0060)).GetByteValue();
    if (bv)
    {
      std::string modality = std::string(bv->GetPointer(), bv->GetLength());
      GuessFromModality(modality.c_str());
    }
  }
  if (MSField == MediaStorage::MS_END)
  {
    // Return a default SC Object
    mdcmWarningMacro("Unknown MediaStorage, but Pixel Data element found");
    MSField = MediaStorage::SecondaryCaptureImageStorage;
    return false;
  }
  return true;
}

bool
MediaStorage::SetFromFile(File const & file)
{
  /*
   * DICOMDIR usually have group 0002 present, but no 0008,0016 (doh!)
   * So we first check in header, if found, assumed it is ok (we should
   * check that consistent with 0008,0016 ...)
   * A lot of DICOM image file are still missing the group header
   * this is why we check 0008,0016, and to preserve compat with ACR-NEMA
   * we also check Modality element to guess a fake Media Storage UID
   * file such as:
   * mdcmData/SIEMENS-MR-RGB-16Bits.dcm are a pain to handle
   */
  const FileMetaInformation & header = file.GetHeader();
  std::string                 b1;
  const char *                header_ms_ptr = GetFromHeader(header, b1);
  std::string                 copy1;
  const char *                header_ms_str = NULL;
  if (header_ms_ptr)
  {
    copy1 = header_ms_ptr;
    header_ms_str = copy1.c_str();
  }
  const DataSet & ds = file.GetDataSet();
  std::string     b2;
  const char *    ds_ms_ptr = GetFromDataSet(ds, b2);
  std::string     copy2;
  const char *    ds_ms_str = NULL;
  if (ds_ms_ptr)
  {
    copy2 = ds_ms_ptr;
    ds_ms_str = copy2.c_str();
  }
  if (header_ms_str && ds_ms_str && strcmp(header_ms_str, ds_ms_str) == 0)
  {
    return SetFromHeader(header);
  }
  if (ds_ms_str)
  {
    // Means either no header ms or different, take from dataset
    return SetFromDataSet(ds);
  }
  // Looks suspicious or DICOMDIR
  if (header_ms_str)
  {
    return SetFromHeader(header);
  }
  if (!SetFromHeader(header))
  {
    // Try again but from dataset this time
    mdcmDebugMacro("No MediaStorage found in Header, looking up in DataSet");
    if (!SetFromDataSet(ds))
    {
      // ACR-NEMA compat
      mdcmDebugMacro("No MediaStorage found neither in DataSet nor in FileMetaInformation, trying from Modality");
      // Attempt to read modality
      if (!SetFromModality(ds))
      {
        return false;
      }
    }
  }
  return true;
}

} // end namespace mdcm
