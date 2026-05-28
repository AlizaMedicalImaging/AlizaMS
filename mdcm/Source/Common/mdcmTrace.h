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
#ifndef MDCMTRACE_H
#define MDCMTRACE_H

#include <iosfwd>
#include <cassert>
#include <iostream>

// clang-format off
namespace mdcm
{

#ifdef MDCM_CXX_HAS_FUNCTION
#  ifdef __BORLANDC__
#    define __FUNCTION__ __FUNC__
#  endif
#  ifdef __GNUC__
#    define MDCM_FUNCTION __PRETTY_FUNCTION__
#  else
#    define MDCM_FUNCTION __FUNCTION__
#  endif
#else
#  define MDCM_FUNCTION "<unknown>"
#endif

// The 'MDCM_NO_MESSAGES' option is not intended for release builds, but e.g.
// for compilation with flags like e.g. '-g -fsanitize=undefined' or the use
// of other debugging methods, to reduce excessive output in debug mode.
// It is intended exclusively for use within this file and will become
// undefined at the end of the file.
//
//#define MDCM_NO_MESSAGES
//
//
//

#ifdef MDCM_NO_MESSAGES

#define mdcmDebugMacro(msg) {}
#define mdcmWarningMacro(msg) {}
#define mdcmErrorMacro(msg) {}
#define mdcmAlwaysWarnMacro(msg) {}

#else // MDCM_NO_MESSAGES

#ifdef NDEBUG

#define mdcmDebugMacro(msg) {}
#define mdcmWarningMacro(msg) {}
#define mdcmErrorMacro(msg) {}

#else // NDEBUG

#define mdcmDebugMacro(msg)                                                                           \
 {                                                                                                    \
   std::ostringstream osmacro;                                                                        \
   osmacro << "Debug: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << '\n'   \
           << msg;                                                                                    \
   std::cout << osmacro.str() << "\n\n" << std::endl;                                                 \
 }

#define mdcmWarningMacro(msg)                                                                         \
 {                                                                                                    \
   std::ostringstream osmacro;                                                                        \
   osmacro << "Warning: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << '\n' \
           << msg << "\n\n";                                                                          \
   std::cout << osmacro.str() << std::endl;                                                           \
 }

#define mdcmErrorMacro(msg)                                                                           \
 {                                                                                                    \
   std::ostringstream osmacro;                                                                        \
   osmacro << "Error: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << '\n'   \
           << msg << "\n\n";                                                                          \
   std::cout << osmacro.str() << std::endl;                                                           \
 }

#endif // NDEBUG

#define mdcmAlwaysWarnMacro(msg)             \
 {                                           \
   std::ostringstream osmacro;               \
   osmacro << "Warning:\n" << msg << "\n\n"; \
   std::cout << osmacro.str() << std::endl;  \
 }

#endif // MDCM_NO_MESSAGES

#ifdef MDCM_NO_MESSAGES
#undef MDCM_NO_MESSAGES
#endif

}

#endif // MDCMTRACE_H
