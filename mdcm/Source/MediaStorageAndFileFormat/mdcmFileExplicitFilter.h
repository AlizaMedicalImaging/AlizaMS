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
#ifndef MDCMFILEEXPLICITFILTER_H
#define MDCMFILEEXPLICITFILTER_H

#include "mdcmFile.h"

namespace mdcm
{

class Dicts;

class MDCM_EXPORT FileExplicitFilter
{
public:
  FileExplicitFilter()
    : F(new File)
    , ChangePrivateTags(false)
    , UseVRUN(true)
    , RecomputeItemLength(false)
    , RecomputeSequenceLength(false)
  {}
  ~FileExplicitFilter() {}
  void
  SetChangePrivateTags(bool);
  void
  SetUseVRUN(bool);
  void
  SetRecomputeItemLength(bool);
  void
  SetRecomputeSequenceLength(bool);
  void
  SetFile(const File &);
  File &
  GetFile();
  bool
  Change();

protected:
  bool
  ChangeFMI();
  bool
  ProcessDataSet(DataSet &, Dicts const &);

private:
  SmartPointer<File> F;
  bool               ChangePrivateTags;
  bool               UseVRUN;
  bool               RecomputeItemLength;
  bool               RecomputeSequenceLength;
};


} // end namespace mdcm

#endif // MDCMFILEEXPLICITFILTER_H
