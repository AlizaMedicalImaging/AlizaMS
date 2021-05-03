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

#ifndef MDCMOBJECT_H
#define MDCMOBJECT_H

#include "mdcmTypes.h"
#include "mdcmTrace.h"
#include <iostream>

namespace mdcm
{

template <class ObjectType>
class SmartPointer;

class MDCM_EXPORT Object
{
  friend std::ostream &
  operator<<(std::ostream &, const Object &);
  template <class ObjectType>
  friend class SmartPointer;

public:
  Object()
    : ReferenceCount(0)
  {}

  virtual ~Object() { assert(ReferenceCount == 0); }

  Object(const Object &)
    : ReferenceCount(0)
  {}

  void
  operator=(const Object &)
  {}

protected:
  void
  Register()
  {
    ++ReferenceCount;
    assert(ReferenceCount > 0);
  }

  void
  UnRegister()
  {
    assert(ReferenceCount > 0);
    --ReferenceCount;
    if (ReferenceCount <= 0)
    {
      assert(ReferenceCount == 0);
      delete this;
    }
  }

public:
  virtual void
  Print(std::ostream &) const
  {}

private:
  long long ReferenceCount;
};

// Define in the base class the operator and use the member function
// ->Print() to call the appropriate function.
// All subclass of Object needs to implement the Print function
inline std::ostream &
operator<<(std::ostream & os, const Object & obj)
{
  obj.Print(os);
  return os;
}

} // end namespace mdcm

#endif // MDCMOBJECT_H
