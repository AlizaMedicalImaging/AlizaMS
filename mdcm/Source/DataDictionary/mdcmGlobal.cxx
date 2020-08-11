/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmGlobal.h"
#include "mdcmDicts.h"

namespace mdcm
{

unsigned int GlobalCount;

class GlobalInternal
{
public:
  GlobalInternal():GlobalDicts() {}
  Dicts GlobalDicts;
};

Global::Global()
{
  if(++GlobalCount == 1)
  {
    Internals = new GlobalInternal;
    Internals->GlobalDicts.LoadDefaults();
  }
}

Global::~Global()
{
  if(--GlobalCount == 0)
  {
    delete Internals;
    Internals = NULL;
  }
}

Dicts const &Global::GetDicts() const
{
  return Internals->GlobalDicts;
}

Dicts &Global::GetDicts()
{
  return Internals->GlobalDicts;
}

Global& Global::GetInstance()
{
  return GlobalInstance;
}

GlobalInternal * Global::Internals;

} // end namespace mdcm
