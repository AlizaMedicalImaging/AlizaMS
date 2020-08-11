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

class MDCM_EXPORT Trace
{
public :
  Trace();
  ~Trace();

  // Explicitly set the ostream for mdcm::Trace to report to
  // This will set the DebugStream, WarningStream and ErrorStream at once:
  static void SetStream(std::ostream & os);
  static std::ostream & GetStream();

  // Explicitly set the stream which receive Debug messages:
  static void SetDebugStream(std::ostream & os);
  static std::ostream & GetDebugStream();

  // Explicitly set the stream which receive Warning messages:
  static void SetWarningStream(std::ostream & os);
  static std::ostream & GetWarningStream();

  // Explicitly set the stream which receive Error messages:
  static void SetErrorStream(std::ostream & os);
  static std::ostream & GetErrorStream();

  // Explicitly set the filename for mdcm::Trace to report to
  // The file will be created (it will not append to existing file)
  static void SetStreamToFile(const char * filename);

  // Turn debug messages on (default: false)
  static void SetDebug(bool debug);
  static void DebugOn();
  static void DebugOff();
  static bool GetDebugFlag();

  // Turn warning messages on (default: true)
  static void SetWarning(bool debug);
  static void WarningOn();
  static void WarningOff();
  static bool GetWarningFlag();

  // Turn error messages on (default: true)
  static void SetError(bool debug);
  static void ErrorOn();
  static void ErrorOff();
  static bool GetErrorFlag();

};

// Here we define function this is the only way to be able to pass
// stuff with indirection like:
// mdcmDebug("my message:" << i << '\t');
// You cannot use function unless you use vnsprintf ...

// __FUNCTION is not always defined by preprocessor
// In c++ we should use __PRETTY_FUNCTION__ instead...
#ifdef MDCM_CXX_HAS_FUNCTION
// Handle particular case for GNU C++ which also defines __PRETTY_FUNCTION__
// which is a lot nice in C++
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

#if defined(NDEBUG)
#define mdcmDebugMacro(msg) {}
#else
#define mdcmDebugMacro(msg)                                      \
{                                                                \
   if(mdcm::Trace::GetDebugFlag())                               \
   {                                                             \
   std::ostringstream osmacro;                                   \
   osmacro << "Debug: In " __FILE__ ", line " << __LINE__        \
           << ", function " << MDCM_FUNCTION << '\n'             \
           << "Last system error was: "                          \
           << mdcm::System::GetLastSystemError() << '\n' << msg; \
   std::ostream &_os = mdcm::Trace::GetDebugStream();            \
   _os << osmacro.str() << "\n\n" << std::endl;                  \
   }                                                             \
}
#endif //NDEBUG

/**
 * \brief   Warning
 * @param msg message part
 */
#if defined(NDEBUG)
#define mdcmWarningMacro(msg) {}
#else
#define mdcmWarningMacro(msg)                               \
{                                                           \
   if(mdcm::Trace::GetWarningFlag())                        \
   {                                                        \
   std::ostringstream osmacro;                              \
   osmacro << "Warning: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION << "\n"        \
           << msg << "\n\n";                                \
   std::ostream &_os = mdcm::Trace::GetWarningStream();     \
   _os << osmacro.str() << std::endl;                       \
   }                                                        \
}
#endif //NDEBUG

/**
 * \brief   Error this is pretty bad, more than just warning
 * It could mean lost of data, something not handle...
 * @param msg second message part
 */
#if defined(NDEBUG)
#define mdcmErrorMacro(msg) {}
#else
#define mdcmErrorMacro(msg)                               \
{                                                         \
   if(mdcm::Trace::GetErrorFlag())                        \
   {                                                      \
   std::ostringstream osmacro;                            \
   osmacro << "Error: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION << '\n'      \
           << msg << "\n\n";                              \
   std::ostream &_os = mdcm::Trace::GetErrorStream();     \
   _os << osmacro.str() << std::endl;                     \
   }                                                      \
}
#endif //NDEBUG

/**
 * \brief   Assert
 * @param arg argument to test
 *        An easy solution to pass also a message is to do:
 *        mdcmAssertMacro("my message" && 2 < 3)
 */
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
   std::ostream &_os = mdcm::Trace::GetErrorStream();      \
   _os << osmacro.str() << std::endl;                      \
   assert(arg);                                            \
   }                                                       \
}
#endif //NDEBUG

/**
 * \brief   AssertAlways
 * @param arg argument to test
 *        An easy solution to pass also a message is to do:
 *        mdcmAssertMacro("my message" && 2 < 3)
 */
#if defined(NDEBUG)
// User asked for release compilation, but still need to report
// if grave issue.
#define mdcmAssertAlwaysMacro(arg) \
{                                                          \
   if(!(arg))                                              \
   {                                                       \
   std::ostringstream osmacro;                             \
   osmacro << "Assert: In " __FILE__ ", line " << __LINE__ \
           << ", function " << MDCM_FUNCTION               \
           << "\n\n";                                      \
   throw osmacro.str();                                    \
   }                                                       \
}
#else
// Simply reproduce mdcmAssertMacro behavior
#define mdcmAssertAlwaysMacro(arg) mdcmAssertMacro(arg)
#endif //NDEBUG

#else // MDCM_SUPRESS_OUTPUT

#define mdcmDebugMacro(msg) {}
#define mdcmWarningMacro(msg) {}
#define mdcmErrorMacro(msg) {}
#define mdcmAssertMacro(arg) {}
#define mdcmAssertAlwaysMacro(arg) {}

#endif // MDCM_SUPRESS_OUTPUT

#define mdcmAlwaysWarnMacro(msg)                        \
{                                                       \
   if(mdcm::Trace::GetWarningFlag())                    \
   {                                                    \
   std::ostringstream osmacro;                          \
   osmacro << "Warning:\n" << msg << "\n\n";            \
   std::ostream &_os = mdcm::Trace::GetWarningStream(); \
   _os << osmacro.str() << std::endl;                   \
   }                                                    \
}

} // end namespace mdcm

#endif //MDCMTRACE_H
