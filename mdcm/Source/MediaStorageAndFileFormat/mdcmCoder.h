/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMCODER_H
#define MDCMCODER_H

#include "mdcmTypes.h"
#include "mdcmDataElement.h"

namespace mdcm
{

class TransferSyntax;
class DataElement;
/**
 * \brief Coder
 */
class MDCM_EXPORT Coder
{
public:
  virtual ~Coder() {}
  virtual bool CanCode(TransferSyntax const &) const = 0;
  virtual bool Code(DataElement const &, DataElement &)
  {
    return false;
  }
protected:
  virtual bool InternalCode(const char *, unsigned long, std::ostream &)
  {
    return false;
  }
};

} // end namespace mdcm

#endif //MDCMCODER_H
