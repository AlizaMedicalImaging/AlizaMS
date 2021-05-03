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
#ifndef MDCMVALUEIO_H
#define MDCMVALUEIO_H

#include "mdcmTypes.h"

namespace mdcm
{
/**
 * Class to dispatch template calls
 */
template <typename TDE, typename TSwap, typename TType = uint8_t>
class ValueIO
{
public:
  static std::istream &
  Read(std::istream &, Value &, bool);
  static const std::ostream &
  Write(std::ostream &, const Value &);
};

} // namespace mdcm

#include "mdcmValueIO.hxx"

#endif // MDCMVALUEIO_H
