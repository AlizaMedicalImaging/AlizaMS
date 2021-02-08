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
public:
  Global();
  ~Global();
  const Dicts & GetDicts() const;
  Dicts & GetDicts();
  static Global & GetInstance();

private:
  Global(const Global &);
  static GlobalInternal * Internals;
};

static Global GlobalInstance;

}

#endif //MDCMGLOBAL_H
