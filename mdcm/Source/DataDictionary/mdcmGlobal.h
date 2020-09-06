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

#ifndef MDCMGLOBAL_H
#define MDCMGLOBAL_H

#include "mdcmTypes.h"

namespace mdcm
{
class GlobalInternal;
class Dicts;
class MDCM_EXPORT Global
{
  friend std::ostream& operator<<(std::ostream & _os, const Global & g);
public:
  Global();
  ~Global();
  Dicts const & GetDicts() const;
  Dicts & GetDicts();
  static Global & GetInstance();
private:
  Global &operator=(const Global &_val);
  Global(const Global &_val);
  static GlobalInternal * Internals;
};

inline std::ostream& operator<<(std::ostream &os, const Global &g)
{
  (void)g;
  return os;
}

static Global GlobalInstance;

}

#endif //MDCMGLOBAL_H
