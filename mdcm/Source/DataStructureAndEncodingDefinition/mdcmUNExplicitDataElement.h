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
#ifndef MDCMUNEXPLICITDATAELEMENT_H
#define MDCMUNEXPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm
{
/**
 * \brief Class to read/write a DataElement as UNExplicit Data Element
 */
class MDCM_EXPORT UNExplicitDataElement : public DataElement
{
public:
  VL GetLength() const;
  template <typename TSwap>
  std::istream & Read(std::istream &);
  template <typename TSwap>
  std::istream & ReadPreValue(std::istream &);
  template <typename TSwap>
  std::istream & ReadValue(std::istream &, bool = true);
  template <typename TSwap>
  std::istream & ReadWithLength(std::istream &, VL &);
  // purposely do not provide an implementation for writing
};

}

#include "mdcmUNExplicitDataElement.txx"

#endif //MDCMUNEXPLICITDATAELEMENT_H
