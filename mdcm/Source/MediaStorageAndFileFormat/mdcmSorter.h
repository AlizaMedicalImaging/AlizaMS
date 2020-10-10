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
#ifndef MDCMSORTER_H
#define MDCMSORTER_H

#include "mdcmTag.h"
#include "mdcmSmartPointer.h"
#include "mdcmFile.h"
#include <vector>
#include <string>
#include <map>
#include <set>

namespace mdcm
{

class DataSet;

class MDCM_EXPORT FileWithName : public File
{
public:
  FileWithName(File & f) : File(f), filename() {}
  std::string filename;
};

typedef std::vector< SmartPointer<FileWithName> > FileList;
typedef bool (*BOOL_FUNCTION_PFILE_PFILE_POINTER)(File *, File *);

/**
 * Sorter
 * General class to do sorting using a custom function
 * You simply need to provide a function of type: Sorter::SortFunction
 *
 * There is no cache mechanism. Which means
 * that everytime you call Sort, all files specified as input paramater are *read*
 *
 */
class MDCM_EXPORT Sorter
{
  friend std::ostream& operator<<(std::ostream &, const Sorter &);
public:
  Sorter();
  virtual ~Sorter();
  virtual bool Sort(std::vector<std::string> const &);
  const std::vector<std::string> &GetFilenames() const { return Filenames; }
  void Print(std::ostream &) const;
  bool AddSelect(Tag const &, const char *);
  void SetTagsToRead(std::set<Tag> const &);
  typedef bool (*SortFunction)(DataSet const &, DataSet const &);
  void SetSortFunction(SortFunction);
  virtual bool StableSort(std::vector<std::string> const &);

protected:
  std::vector<std::string> Filenames;
  typedef std::map<Tag,std::string> SelectionMap;
  std::map<Tag,std::string> Selection;
  SortFunction SortFunc;
  std::set<Tag> TagsToRead;
};

inline std::ostream& operator<<(std::ostream &os, const Sorter & s)
{
  s.Print(os);
  return os;
}


} // end namespace mdcm

#endif //MDCMSORTER_H
