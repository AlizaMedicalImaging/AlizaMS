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
#ifndef MDCMUIDGENERATOR_H
#define MDCMUIDGENERATOR_H

#include "mdcmTypes.h"

namespace mdcm
{

class MDCM_EXPORT UIDGenerator
{
public:
  UIDGenerator() = default;
  ~UIDGenerator() = default;
  static void
  SetRoot(const char *);
  static const char *
  GetRoot();
  const char *
  Generate();
  static bool
  IsValid(const std::string &);
  static const char *
  GetMDCMUID();

protected:
  static bool
  GenerateUUID(unsigned char *);

private:
  static const char  MDCM_UID[];
  static std::string Root;
  static std::string EncodedHardwareAddress;
  std::string        Unique{};
};

} // end namespace mdcm

#endif // MDCMUIDGENERATOR_H
