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
#  define MDCMTRACE_H

#  include <iosfwd>
#  include <cassert>
#  include <iostream>

// clang-format off
namespace mdcm
{

#  ifdef MDCM_CXX_HAS_FUNCTION
#    ifdef __BORLANDC__
#      define __FUNCTION__ __FUNC__
#    endif
#    ifdef __GNUC__
#      define MDCM_FUNCTION __PRETTY_FUNCTION__
#    else
#      define MDCM_FUNCTION __FUNCTION__
#    endif
#  else
#    define MDCM_FUNCTION "<unknown>"
#  endif

#  ifdef NDEBUG

#    define mdcmDebugMacro(msg) {} 
#    define mdcmWarningMacro(msg) {}
#    define mdcmErrorMacro(msg) {}

#  else

#    define mdcmDebugMacro(msg)                                                                             \
     {                                                                                                      \
       std::ostringstream osmacro;                                                                          \
       osmacro << "Debug: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << "\n"     \
               << msg;                                                                                      \
       std::cout << osmacro.str() << "\n\n" << std::endl;                                                   \
     }

#    define mdcmWarningMacro(msg)                                                                           \
     {                                                                                                      \
       std::ostringstream osmacro;                                                                          \
       osmacro << "Warning: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << "\n"   \
               << msg << "\n\n";                                                                            \
       std::cout << osmacro.str() << std::endl;                                                             \
     }

#    define mdcmErrorMacro(msg)                                                                             \
     {                                                                                                      \
       std::ostringstream osmacro;                                                                          \
       osmacro << "Error: in " __FILE__ ", line " << __LINE__ << ", function " << MDCM_FUNCTION << "\n"     \
               << msg << "\n\n";                                                                            \
       std::cout << osmacro.str() << std::endl;                                                             \
     }

#  endif

#  define mdcmAlwaysWarnMacro(msg)             \
   {                                           \
     std::ostringstream osmacro;               \
     osmacro << "Warning:\n" << msg << "\n\n"; \
     std::cout << osmacro.str() << std::endl;  \
   }

} // end namespace mdcm
// clang-format on

#endif // MDCMTRACE_H
