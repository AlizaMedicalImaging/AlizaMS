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
#ifndef MDCMFILEMETAINFORMATION_H
#define MDCMFILEMETAINFORMATION_H

#include "mdcmPreamble.h"
#include "mdcmDataSet.h"
#include "mdcmDataElement.h"
#include "mdcmMediaStorage.h"
#include "mdcmTransferSyntax.h"
#include "mdcmExplicitDataElement.h"

namespace mdcm
{
/**
 * Class to represent a File Meta Information
 *
 * FileMetaInformation is a Explicit Structured Set.  Whenever the
 * file contains an ImplicitDataElement DataSet, a conversion will take place.
 *
 * Definition:
 * The File Meta Information includes identifying information on the
 * encapsulated Data Set. This header consists of a 128 byte File Preamble,
 * followed by a 4 byte DICOM prefix, followed by the File Meta Elements shown
 * in Table 7.1-1. This header shall be present in every DICOM file.
 *
 */
class MDCM_EXPORT FileMetaInformation : public DataSet
{
friend std::ostream &operator<<(std::ostream &, const FileMetaInformation &);
public:
  FileMetaInformation();
  FileMetaInformation(FileMetaInformation const &);
  ~FileMetaInformation();

  FileMetaInformation& operator=(const FileMetaInformation & fmi)
  {
    DataSetTS = fmi.DataSetTS;
    MetaInformationTS = fmi.MetaInformationTS;
    DataSetMS = fmi.DataSetMS;
    return *this;
  }

  bool IsValid() const;
  TransferSyntax::NegociatedType GetMetaInformationTS() const;
  void SetDataSetTransferSyntax(const TransferSyntax &);
  const TransferSyntax & GetDataSetTransferSyntax() const;
  std::string GetMediaStorageAsString() const;
  MediaStorage GetMediaStorage() const;
  void Insert(const DataElement &);
  void Replace(const DataElement &);
  static void SetImplementationClassUID(const char *);
  static void AppendImplementationClassUID(const char *);
  static void SetImplementationVersionName(const char *);
  static void SetSourceApplicationEntityTitle(const char *);
  static const char * GetImplementationClassUID();
  static const char * GetImplementationVersionName();
  static const char * GetSourceApplicationEntityTitle();
  bool FillFromDataSet(const DataSet &);
  bool Read2(std::istream &);
  bool ReadCompat2(std::istream &);
  bool Write2(std::ostream &) const;
  const Preamble & GetPreamble() const;
  Preamble & GetPreamble();
  void SetPreamble(const Preamble &);
  VL GetFullLength() const;

protected:
  template <typename TSwap> bool ReadCompatInternal2(std::istream &);
  bool ComputeDataSetTransferSyntax();
  static const char * GetFileMetaInformationVersion();
  static const char * GetMDCMImplementationClassUID();
  static const char * GetMDCMImplementationVersionName();
  static const char * GetMDCMSourceApplicationEntityTitle();

  TransferSyntax DataSetTS;
  TransferSyntax::NegociatedType MetaInformationTS;
  MediaStorage::MSType DataSetMS;

private:
  Preamble P;
  static const char MDCM_FILE_META_INFORMATION_VERSION[];
  static const char MDCM_IMPLEMENTATION_CLASS_UID[];
  static const char MDCM_IMPLEMENTATION_VERSION_NAME[];
  static const char MDCM_SOURCE_APPLICATION_ENTITY_TITLE[];
  static std::string ImplementationClassUID;
  static std::string ImplementationVersionName;
  static std::string SourceApplicationEntityTitle;
};

inline std::ostream& operator<<(std::ostream & os, const FileMetaInformation & val)
{
  val.GetPreamble().Print(os);
  val.Print(os);
  return os;
}

} // end namespace mdcm

#endif //MDCMFILEMETAINFORMATION_H
