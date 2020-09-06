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
#ifndef MDCMVR16EXPLICITDATAELEMENT_H
#define MDCMVR16EXPLICITDATAELEMENT_H

#include "mdcmDataElement.h"

namespace mdcm
{
/**
 * \brief Class to read/write a DataElement as Explicit Data Element
 * \note This class support 16 bits when finding an unkown VR:
 * For instance:
 * Siemens_CT_Sensation64_has_VR_RT.dcm
 */
class MDCM_EXPORT VR16ExplicitDataElement : public DataElement
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
  // Purposely do not provide an implementation for writing!
};

}

#include "mdcmVR16ExplicitDataElement.txx"

#endif //MDCMVR16EXPLICITDATAELEMENT_H
