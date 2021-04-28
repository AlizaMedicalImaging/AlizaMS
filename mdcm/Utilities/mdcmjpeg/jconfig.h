/* jconfig.h --- source file edited by configure script */
/* see jconfig.doc for explanations */

#undef HAVE_PROTOTYPES
#undef HAVE_UNSIGNED_CHAR
#undef HAVE_UNSIGNED_SHORT
#undef void
#undef const
#undef CHAR_IS_UNSIGNED
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
/* Define this if you get warnings about undefined structures. */
#undef INCOMPLETE_TYPES_BROKEN


#if defined(_WIN32) && !(defined(__CYGWIN__) || defined(__MINGW32__))
/* Define "boolean" as unsigned char, not int, per Windows custom */
/* don't conflict if rpcndr.h already read; Note that the w32api headers
   used by Cygwin and Mingw do not define "boolean", so jmorecfg.h
   handles it later. */
#  ifndef __RPCNDR_H__
typedef unsigned char boolean;
#  endif
#  define HAVE_BOOLEAN /* prevent jmorecfg.h from redefining it */
#endif

#ifdef JPEG_INTERNALS

#  undef RIGHT_SHIFT_IS_UNSIGNED
/* Don't use getenv */
#  define NO_GETENV

#endif

#include "jpegcmake.h"
