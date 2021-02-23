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

#ifndef MDCMWRITER_H
#define MDCMWRITER_H

#include "mdcmFile.h"

namespace mdcm
{

class FileMetaInformation;
/**
 * Writer ala DOM (Document Object Model)
 * This class is a non-validating writer, it will only performs well-
 * formedness check only.
 *
 * Detailled description here
 * To avoid MDCM being yet another broken DICOM lib we try to
 * be user level and avoid writing illegal stuff (odd length,
 * non-zero value for Item start/end length ...)
 * Therefore you cannot (well unless you are really smart) write
 * DICOM with even length tag.
 * All the checks are consider basics:
 * - Correct Meta Information Header (see mdcm::FileMetaInformation)
 * - Zero value for Item Length (0xfffe, 0xe00d/0xe0dd)
 * - Even length for any elements
 * - Alphabetical order for elements (garanteed by design of internals)
 * - 32bits VR will be rewritten with 00
 *
 * mdcm::Writer cannot write a DataSet if no SOP Instance UID (0008,0018) is found,
 * unless a DICOMDIR is being written out
 *
 */
class MDCM_EXPORT Writer
{
friend class StreamImageWriter;
public:
  Writer();
  virtual ~Writer();
  virtual bool Write();
  void SetFileName(const char *);
  void SetStream(std::ostream &);
  void SetFile(const File &);
  File & GetFile();
  void SetCheckFileMetaInformation(bool);
  void CheckFileMetaInformationOff();
  void CheckFileMetaInformationOn();
protected:
  void SetWriteDataSetOnly(bool);
  std::ostream * GetStreamPtr() const;
  bool GetCheckFileMetaInformation() const;
  std::ostream * Stream;
  std::ofstream * Ofstream;
private:
  SmartPointer<File> F;
  bool CheckFileMetaInformation;
  bool WriteDataSetOnly;
};

} // end namespace mdcm

#endif //MDCMWRITER_H
