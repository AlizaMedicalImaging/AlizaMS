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
#ifndef MDCMCP246EXPLICITDATAELEMENT_H
#define MDCMCP246EXPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm
{
// Data Element (CP246Explicit)
/**
 * \brief Class to read/write a DataElement as CP246Explicit Data Element
 * \details 
 * \note Some system are producing SQ, declare them as UN, but encode the SQ as 'Explicit'
 * instead of Implicit
 */
class MDCM_EXPORT CP246ExplicitDataElement : public DataElement
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
  std::istream & ReadWithLength(std::istream & is, VL & length);

  // PURPOSELY do not provide an implementation for writing
};

} // end namespace mdcm

#include "mdcmCP246ExplicitDataElement.txx"

#endif //MDCMCP246EXPLICITDATAELEMENT_H
