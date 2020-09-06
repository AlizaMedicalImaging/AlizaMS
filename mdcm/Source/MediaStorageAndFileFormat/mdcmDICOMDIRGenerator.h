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
#ifndef MDCMDICOMDIRGENERATOR_H
#define MDCMDICOMDIRGENERATOR_H

#include "mdcmDirectory.h"
#include "mdcmTag.h"
#include <utility>
#include <string>

namespace mdcm
{
class File;
class Scanner;
class SequenceOfItems;
class VL;
class DICOMDIRGeneratorInternal;

/**
 * \brief DICOMDIRGenerator class
 * \details This is a STD-GEN-CD DICOMDIR generator.
 * ref: PS 3.11-2008 Annex D (Normative) - General Purpose CD-R and DVD Interchange Profiles
 *
 * \note
 * PS 3.11 - 2008 / D.3.2 Physical Medium And Medium Format
 * The STD-GEN-CD and STD-GEN-SEC-CD application profiles require the 120 mm CD-R physical
 * medium with the ISO/IEC 9660 Media Format, as defined in PS3.12.
 * See also PS 3.12 - 2008 / Annex F 120mm CD-R Medium (Normative) and
 * PS 3.10 - 2008 / 8 DICOM File Service / 8.1 FILE-SET
 *
 * \warning:
 * PS 3.11 - 2008 / D.3.1 SOP Classes and Transfer Syntaxes
 * Composite Image & Stand-alone Storage are required to be stored as Explicit VR Little
 * Endian Uncompressed (1.2.840.10008.1.2.1). When a DICOM file is found using another
 * Transfer Syntax the generator will simply stops.
 *
 * \warning
 * - Input files should be Explicit VR Little Endian
 * - filenames should be valid VR::CS value (16 bytes, upper case ...)
 *
 * \bug:
 * There is a current limitation of not handling Referenced SOP Class UID /
 * Referenced SOP Instance UID simply because the Scanner does not allow us
 * See PS 3.11 / Table D.3-2 STD-GEN Additional DICOMDIR Keys
 */
class MDCM_EXPORT DICOMDIRGenerator
{
  typedef std::pair<std::string, Tag> MyPair;

public:
  typedef Directory::FilenamesType  FilenamesType;
  typedef Directory::FilenameType  FilenameType;
  DICOMDIRGenerator();
  ~DICOMDIRGenerator();
  void SetFilenames(FilenamesType const & fns);
  void SetRootDirectory(FilenameType const & root);
  void SetDescriptor(const char * d);
  bool Generate();
  void SetFile(const File& f);
  File & GetFile();

protected:
  Scanner & GetScanner();
  bool AddPatientDirectoryRecord();
  bool AddStudyDirectoryRecord();
  bool AddSeriesDirectoryRecord();
  bool AddImageDirectoryRecord();

private:
  const char * ComputeFileID(const char *);
  bool TraverseDirectoryRecords(VL start);
  bool ComputeDirectoryRecordsOffset(
    const SequenceOfItems * sqi,
    VL start);
  size_t FindNextDirectoryRecord(
    size_t item1,
    const char * directorytype);
  SequenceOfItems * GetDirectoryRecordSequence();
  size_t FindLowerLevelDirectoryRecord(
   size_t item1,
   const char * directorytype);
  MyPair GetReferenceValueForDirectoryType(size_t item);
  bool SeriesBelongToStudy(
    const char * seriesuid,
    const char * studyuid);
  bool ImageBelongToSeries(
   const char * sopuid,
   const char * seriesuid,
   Tag const & t1,
   Tag const & t2);
  bool ImageBelongToSameSeries(
   const char * sopuid,
   const char * seriesuid,
   Tag const & t);
  DICOMDIRGeneratorInternal * Internals;
};

} // end namespace mdcm

#endif //MDCMDICOMDIRGENERATOR_H
