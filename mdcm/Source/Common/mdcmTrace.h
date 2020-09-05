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

// TODO
#define MDCM_SUPRESS_OUTPUT

#include "mdcmTypes.h"
#include "mdcmSystem.h"
#include <iosfwd>
#include <cassert>

namespace mdcm
{

#ifdef MDCM_CXX_HAS_FUNCTION
// Handle particular case for GNU C++ which also defines __PRETTY_FUNCTION__
#ifdef __BORLANDC__
#  define __FUNCTION__ __FUNC__
#endif
#ifdef __GNUC__
#  define MDCM_FUNCTION __PRETTY_FUNCTION__
#else
#  define MDCM_FUNCTION __FUNCTION__
#endif //__GNUC__
#else
#  define MDCM_FUNCTION "<unknown>"
#endif //MDCM_CXX_HAS_FUNCTION

#ifdef MDCM_SUPRESS_OUTPUT

#define mdcmDebugMacro(msg) {}
#define mdcmWarningMacro(msg) {}
#define mdcmErrorMacro(msg) {}
#define mdcmAssertMacro(arg) {}
#define mdcmAssertAlwaysMacro(arg) {}

#else // MDCM_SUPRESS_OUTPUT

#if defined(NDEBUG)
#define mdcmDebugMacro(msg) {}
#else
#define mdcmDebugMacro(msg)                                      \
{                                                                \
   std::ostringstream osmacro;                                   \
   osmacro << "Debug: In " __FILE__ ", line " << __LINE__        \
           << ", function " << MDCM_FUNCTION << '\n'             \
           << "Last system error was: "                          \
           << mdcm::System::GetLastSystemError() << '\n' << msg; \
   std::cout << osmacro.str() << "\n\n" << std::endl;            \
}
#endif

#if defined(NDEBUG)
#define mdcmWarningMacro(msg) {}
#else
#define mdcmWarningMacro(msg)                               \
{                                                           \
   std::ostringstream osmacro;                              \
   osmacro << "Warning: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION << "\n"        \
           << msg << "\n\n";                                \
   std::cout << osmacro.str() << std::endl;                 \
}
#endif

#if defined(NDEBUG)
#define mdcmErrorMacro(msg) {}
#else
#define mdcmErrorMacro(msg)                               \
{                                                         \
   std::ostringstream osmacro;                            \
   osmacro << "Error: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION << '\n'      \
           << msg << "\n\n";                              \
   std::cout << osmacro.str() << std::endl;               \
}
#endif

#if defined(NDEBUG)
#define mdcmAssertMacro(arg) {}
#else
#define mdcmAssertMacro(arg)                               \
{                                                          \
   if(!(arg))                                              \
   {                                                       \
   std::ostringstream osmacro;                             \
   osmacro << "Assert: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION               \
           << "\n\n";                                      \
   std::cout << osmacro.str() << std::endl;                \
   assert(arg);                                            \
   }                                                       \
}
#endif

#define mdcmAssertAlwaysMacro(arg) mdcmAssertMacro(arg)

#endif // MDCM_SUPRESS_OUTPUT

#define mdcmAlwaysWarnMacro(msg)                        \
{                                                       \
   std::ostringstream osmacro;                          \
   osmacro << "Warning:\n" << msg << "\n\n";            \
   std::cout << osmacro.str() << std::endl;             \
}

} // end namespace mdcm

#endif //MDCMTRACE_H
