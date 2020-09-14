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
#ifndef MDCMEXPLICITIMPLICITDATAELEMENT_H
#define MDCMEXPLICITIMPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm
{
/**
 * Class to read/write a DataElement as ExplicitImplicit Data Element
 * This only happen for some Philips images
 * Should I derive from ExplicitDataElement instead ?
 * This is the class that is the closest the MDCM1.x parser. At each element we try first
 * to read it as explicit, if this fails, then we try again as an implicit element.
 */
class MDCM_EXPORT ExplicitImplicitDataElement : public DataElement
{
public:
  VL GetLength() const;

  template <typename TSwap>
  std::istream & Read(std::istream & is);

  template <typename TSwap>
  std::istream & ReadPreValue(std::istream & is);

  template <typename TSwap>
  std::istream & ReadValue(std::istream & is, bool readvalues = true);

  template <typename TSwap>
  std::istream & ReadWithLength(std::istream & is, VL & length)
  {
    (void)length;
    return Read<TSwap>(is);
  }
  // Purposely do not provide an implementation for writing
};

} // end namespace mdcm

#include "mdcmExplicitImplicitDataElement.txx"

#endif //MDCMEXPLICITIMPLICITDATAELEMENT_H
