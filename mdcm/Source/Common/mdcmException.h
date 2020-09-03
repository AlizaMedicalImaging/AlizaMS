/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMEXCEPTION_H
#define MDCMEXCEPTION_H

#include <cassert>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>

// Disable clang warning "dynamic exception specifications are deprecated".
// We need to be C++03 and C++11 compatible, and if we remove the 'throw()'
// specifier we'll get an error in C++03 by not matching the superclass.
#if defined(__clang__) && defined(__has_warning)
# if __has_warning("-Wdeprecated")
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated"
# endif
#endif

namespace mdcm
{

class Exception : public std::exception
{

  // std::logic_error is used to internally hold a string.
  // It has the nice property of having a copy-constructor
  // that never fails.
  typedef std::logic_error StringHolder;

  static StringHolder CreateWhat(
    const char * const desc,
    const char * const file,
    const unsigned int lineNumber,
    const char * const func)
  {
    assert(desc != NULL);
    assert(file != NULL);
    assert(func != NULL);
    std::ostringstream oswhat;
    oswhat << file << ":" << lineNumber << " (" << func << "):\n";
    oswhat << desc;
    return StringHolder( oswhat.str() );
  }

public:
  // Explicit constructor, initializing the description and the
  // text returned by what().
  // The last parameter is ignored for the time being.
  // It may be used to specify the function where the exception
  // was thrown.
  explicit Exception(
    const char * desc = "None",
    const char * file = __FILE__,
    unsigned int lineNumber = __LINE__,
    // FIXME:  __PRETTY_FUNCTION__ is the non-mangled version
    // for __GNUC__ compiler
    const char * func = "" /*__FUNCTION__*/)
    :
    What(CreateWhat(desc, file, lineNumber, func)),
    Description(desc)  {}

  virtual ~Exception() throw() {}

  const char * what() const throw()
  {
    return What.what();
  }

  const char * GetDescription() const { return Description.what(); }

private:

  StringHolder  What;
  StringHolder  Description;
};

} // end namespace mdcm

// Undo warning suppression.
#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wdeprecated")
#pragma clang diagnostic pop
#endif
#endif

#endif
