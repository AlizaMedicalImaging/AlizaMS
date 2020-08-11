/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMVALIDATE_H
#define MDCMVALIDATE_H

#include "mdcmFile.h"

namespace mdcm
{

/**
 * \brief Validate class
 */
class MDCM_EXPORT Validate
{
public:
  Validate();
  ~Validate();

  void SetFile(File const & f) { F = &f; }
  const File& GetValidatedFile() { return V; }

  void Validation();

protected:
  const File *F;
  File V;
};

} // end namespace mdcm

#endif //MDCMVALIDATE_H
