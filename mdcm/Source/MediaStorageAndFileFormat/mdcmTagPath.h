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
#ifndef MDCMTAGPATH_H
#define MDCMTAGPATH_H

#include "mdcmTag.h"
#include <vector>

namespace mdcm
{

/**
 * \brief class to handle a path of tag
 */
class MDCM_EXPORT TagPath
{
public:
  TagPath();
  ~TagPath();
  void Print(std::ostream &) const;

  // "/0018,0018/"...
  // No space allowed, comma is use to separate tag group
  // from tag element and slash is used to separate tag
  // return false if invalid
  bool ConstructFromString(const char * path);

  static bool IsValid(const char * path);

  bool ConstructFromTagList(Tag const * l, unsigned int n);

  bool Push(Tag const & t);
  bool Push(unsigned int itemnum);

private:
  std::vector<Tag> Path;
};

} // end namespace mdcm

#endif //MDCMTAGPATH_H
