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

#include "mdcmCodeString.h"

namespace mdcm
{

bool
CodeString::IsValid() const
{
  if (!Internal.IsValid())
    return false;
  /*
   * Uppercase characters, 0-9, space character, underscore _, of the
   * Default Character Repertoire
   */
  for (const_iterator it = Internal.cbegin(); it != Internal.cend(); ++it)
  {
    const int c = *it;
    if (!isupper(c) && !isdigit(c) && c != ' ' && c != '_')
    {
      return false;
    }
  }
  return true;
}

}
