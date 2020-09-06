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
#ifndef MDCMIMPLICITDATAELEMENT_H
#define MDCMIMPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm_ns
{

/**
 * \brief Class to represent an *Implicit VR* Data Element
 * \note bla
 */
class MDCM_EXPORT ImplicitDataElement : public DataElement
{
public:
  VL GetLength() const;

  template <typename TSwap>
  std::istream &Read(std::istream& is);

  template <typename TSwap>
  std::istream &ReadPreValue(std::istream& is);

  template <typename TSwap>
  std::istream &ReadValue(std::istream& is, bool readvalues = true);

  template <typename TSwap>
  std::istream &ReadWithLength(std::istream& is, VL & length, bool readvalues = true);

  template <typename TSwap>
  std::istream &ReadValueWithLength(std::istream& is, VL & length, bool readvalues = true);

  template <typename TSwap>
  const std::ostream &Write(std::ostream& os) const;
};

} // end namespace mdcm_ns

#include "mdcmImplicitDataElement.txx"

#endif //MDCMIMPLICITDATAELEMENT_H
