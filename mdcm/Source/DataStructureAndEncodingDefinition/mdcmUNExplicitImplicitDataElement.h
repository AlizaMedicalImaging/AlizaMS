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
#ifndef MDCMUNEXPLICITIMPLICITDATAELEMENT_H
#define MDCMUNEXPLICITIMPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm
{
/**
 * Class to read/write a DataElement as ExplicitImplicit Data Element
 *
 * This class gather two known bugs:
 * 1. GDCM 1.2.0 would rewrite VR=UN Value Length on 2 bytes instead
 *    of 4 bytes
 * 2. GDCM 1.2.0 would also rewrite DataElement as Implicit when the
 *    VR would not be known, this would only happen in some very
 *    rare cases.
 *
 * MDCM could handle bug #1 or #2 exclusively, this class can now
 * handle file which have both issues.
 * See TheralysMDCM120Bug.dcm
 */
class MDCM_EXPORT UNExplicitImplicitDataElement : public DataElement
{
public:
  VL GetLength() const;
  template <typename TSwap>
  std::istream & Read(std::istream &);
  template <typename TSwap>
  std::istream & ReadPreValue(std::istream &);
  template <typename TSwap>
  std::istream & ReadValue(std::istream &);
  // Purposely do not provide an implementation for writing
};

}

#include "mdcmUNExplicitImplicitDataElement.txx"

#endif //MDCMUNEXPLICITIMPLICITDATAELEMENT_H
