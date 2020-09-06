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
#ifndef MDCMEXPLICITDATAELEMENT_H
#define MDCMEXPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm_ns
{
/**
 * \brief Class to read/write a DataElement as Explicit Data Element
 */
class MDCM_EXPORT ExplicitDataElement : public DataElement
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

  template <typename TSwap>
  const std::ostream & Write(std::ostream & os) const;
};

} // end namespace mdcm_ns

#include "mdcmExplicitDataElement.txx"

#endif //MDCMEXPLICITDATAELEMENT_H
