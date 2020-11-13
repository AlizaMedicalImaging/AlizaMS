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
#ifndef MDCMFILENAMEGENERATOR_H
#define MDCMFILENAMEGENERATOR_H

#include "mdcmTypes.h"
#include "mdcmTrace.h"
#include <string>
#include <vector>

namespace mdcm
{
/**
 * FilenameGenerator
 * class to generate filenames based on a pattern (C-style)
 *
 * Output will be:
 *
 * for i = 0, number of filenames:
 *   outfilename[i] = prefix + (pattern % i)
 *
 * where pattern % i means C-style snprintf of Pattern using value 'i'
 */

class MDCM_EXPORT FilenameGenerator
{
public:
  FilenameGenerator():Pattern(),Prefix(),Filenames() {}
  ~FilenameGenerator() {}
  typedef std::string FilenameType;
  typedef std::vector<FilenameType> FilenamesType;
  typedef FilenamesType::size_type  SizeType;

  void SetPattern(const char *pattern) { Pattern = pattern; }
  const char *GetPattern() const { return Pattern.c_str(); }
  void SetPrefix(const char *prefix) { Prefix = prefix; }
  const char *GetPrefix() const { return Prefix.c_str(); }
  bool Generate();
  void SetNumberOfFilenames(SizeType nfiles);
  SizeType GetNumberOfFilenames() const;
  const char * GetFilename(SizeType n) const;
  FilenamesType const & GetFilenames() const { assert( !Pattern.empty() ); return Filenames; }

private:
  FilenameType Pattern;
  FilenameType Prefix;
  FilenamesType Filenames;
};

} // end namespace mdcm

#endif //MDCMFILENAMEGENERATOR_H
