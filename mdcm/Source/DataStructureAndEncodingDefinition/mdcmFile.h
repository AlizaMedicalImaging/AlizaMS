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
#ifndef MDCMFILE_H
#define MDCMFILE_H

#include "mdcmObject.h"
#include "mdcmDataSet.h"
#include "mdcmFileMetaInformation.h"

namespace mdcm
{

/**
 * DICOM File PS 3.10
 */
class MDCM_EXPORT File : public Object
{
public:
  File();
  ~File();
  friend std::ostream &operator<<(std::ostream & os, const File & val);
  std::istream & Read(std::istream & is);
  std::ostream const &Write(std::ostream & os) const;
  const FileMetaInformation &GetHeader() const { return Header; }
  FileMetaInformation & GetHeader() { return Header; }
  void SetHeader( const FileMetaInformation &fmi ) { Header = fmi; }
  const DataSet & GetDataSet() const { return DS; }
  DataSet & GetDataSet() { return DS; }
  void SetDataSet( const DataSet & ds) { DS = ds; }

private:
  FileMetaInformation Header;
  DataSet DS;
};

inline std::ostream& operator<<(std::ostream & os, const File & val)
{
  os << val.GetHeader() << std::endl;
  return os;
}

} // end namespace mdcm

#endif //MDCMFILE_H
