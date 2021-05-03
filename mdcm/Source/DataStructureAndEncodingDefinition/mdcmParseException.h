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
#ifndef MDCMPARSEEXCEPTION_H
#define MDCMPARSEEXCEPTION_H

#include <stdexcept>
#include "mdcmDataElement.h"

namespace mdcm
{

class ParseException : public std::logic_error
{
public:
  explicit ParseException()
    : std::logic_error(std::string(""))
  {}
  explicit ParseException(const std::string & arg)
    : std::logic_error(arg)
  {}
  explicit ParseException(const char * arg)
    : std::logic_error(arg){};
  void
  SetLastElement(const DataElement & de)
  {
    LastElement = de;
  }
  void
  SetLastElement(DataElement & de)
  {
    LastElement = de;
  }
  const DataElement &
  GetLastElement() const
  {
    return LastElement;
  }

private:
  DataElement LastElement;
};

} // end namespace mdcm

#endif
