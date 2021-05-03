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

#ifndef MDCMWIN32_H
#define MDCMWIN32_H

#if !defined(MDCMTYPES_H)
#  error you need to include mdcmTypes.h instead
#endif

// http://gcc.gnu.org/wiki/Visibility
#if defined(_WIN32) && defined(MDCM_BUILD_SHARED_LIBS)
#  if (defined(mdcmCommon_EXPORTS) || defined(mdcmDICT_EXPORTS) || defined(mdcmDSED_EXPORTS) ||                        \
       defined(mdcmMSFF_EXPORTS)
#    define MDCM_EXPORT __declspec(dllexport)
#  else
#    define MDCM_EXPORT __declspec(dllimport)
#  endif
#else
#  if __GNUC__ >= 4 && defined(MDCM_BUILD_SHARED_LIBS)
#    define MDCM_EXPORT __attribute__((visibility("default")))
#    define MDCM_LOCAL __attribute__((visibility("hidden")))
#  else
#    define MDCM_EXPORT
#  endif
#endif

// This is needed when compiling in debug mode
#ifdef _MSC_VER
// to allow construct such as: std::numeric_limits<int>::max() we need the following:
// warning C4003: not enough actual parameters for macro 'max'
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  pragma warning(default : 4263) /* no override, call convention differs */
// 'identifier' : class 'type' needs to have dll-interface to be used by
// clients of class 'type2'
#  pragma warning(disable : 4251)
// non dll-interface class 'type' used as base for dll-interface class 'type2'
#  pragma warning(disable : 4275)
// 'identifier' : identifier was truncated to 'number' characters in the
// debug information
#  pragma warning(disable : 4786)
//'identifier' : decorated name length exceeded, name was truncated
#  pragma warning(disable : 4503)
#endif //_MSC_VER

#endif // MDCMWIN32_H
