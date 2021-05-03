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
#ifndef MDCMTYPES_H
#define MDCMTYPES_H

#include "mdcmConfigure.h"
#include "mdcmWin32.h"
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef MDCM_HAVE_STDINT_H
#  ifndef __STDC_LIMIT_MACROS
#    define __STDC_LIMIT_MACROS 1
#  endif
#  include <stdint.h>
#else
#  ifdef MDCM_HAVE_INTTYPES_H
#    include <inttypes.h>
#  else
#    if defined(__BORLANDC__) && (__BORLANDC__ < 0x0560) || defined(__MINGW32__)
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#    elif defined(_MSC_VER)
#      include "stdint.h"
#    else
#      error "Compiler is not supported"
#    endif
#  endif
#endif

#endif // MDCMTYPES_H
