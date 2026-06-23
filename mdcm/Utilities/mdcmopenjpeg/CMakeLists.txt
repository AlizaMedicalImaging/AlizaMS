if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
  cmake_minimum_required(VERSION 3.13)
endif()

if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

set(OPENJPEG_LIBRARY_NAME mdcmopenjp2)

project(${OPENJPEG_NAMESPACE} C)

set(BUILD_SHARED_LIBS OFF)

include_regular_expression("^.*$")

# OPENJPEG version number
set(OPENJPEG_VERSION_MAJOR 2)
set(OPENJPEG_VERSION_MINOR 5)
set(OPENJPEG_VERSION_BUILD 4)
set(OPENJPEG_VERSION
  "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}")
set(PACKAGE_VERSION
  "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}")

if(NOT OPENJPEG_SOVERSION)
  set(OPENJPEG_SOVERSION 7)
endif(NOT OPENJPEG_SOVERSION)
set(OPENJPEG_LIBRARY_PROPERTIES
  VERSION   "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}"
  SOVERSION "${OPENJPEG_SOVERSION}")

# Path to CMake modules
set(CMAKE_MODULE_PATH
    "${${OPENJPEG_NAMESPACE}_SOURCE_DIR}/cmake"
    ${CMAKE_MODULE_PATH})

set(MANGLE_PREFIX ${MDCM_OPENJPEG_LIBRARIES})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/openjpeg_mangle.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/src/lib/openjp2/openjpeg_mangle.h"
               @ONLY)

# Big endian test
include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(OPJ_BIG_ENDIAN)

# OpenJPEG build configuration options
set(EXECUTABLE_OUTPUT_PATH ${OPENJPEG_BINARY_DIR}/bin CACHE PATH "Single output directory for building all executables.")
set(LIBRARY_OUTPUT_PATH ${OPENJPEG_BINARY_DIR}/bin CACHE PATH "Single output directory for building all libraries.")
mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

# Check if some include files are provided by the system
include(EnsureFileInclude)
# These files are mandatory
ensure_file_include("string.h"   HAVE_STRING_H YES)
ensure_file_include("memory.h"   HAVE_MEMORY_H YES)
ensure_file_include("stdlib.h"   HAVE_STDLIB_H YES)
ensure_file_include("stdio.h"    HAVE_STDIO_H  YES)
ensure_file_include("math.h"     HAVE_MATH_H   YES)
ensure_file_include("float.h"    HAVE_FLOAT_H  YES)
ensure_file_include("time.h"     HAVE_TIME_H   YES)
ensure_file_include("stdarg.h"   HAVE_STDARG_H YES)
ensure_file_include("ctype.h"    HAVE_CTYPE_H  YES)
ensure_file_include("assert.h"   HAVE_ASSERT_H YES)
ensure_file_include("stdint.h"   HAVE_STDINT_H   YES)
ensure_file_include("inttypes.h" HAVE_INTTYPES_H YES)

# For openjpip?
include (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE("strings.h"   HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("sys/stat.h"  HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h"    HAVE_UNISTD_H)

# Large file support
include(TestLargeFiles)
OPJ_TEST_LARGE_FILES(OPJ_HAVE_LARGEFILES)

# Allocating aligned memory blocks
include(CheckIncludeFiles)
check_include_files(malloc.h OPJ_HAVE_MALLOC_H)
include(CheckSymbolExists)
# _aligned_alloc https://msdn.microsoft.com/en-us/library/8z34s9c6.aspx
check_symbol_exists(_aligned_malloc malloc.h OPJ_HAVE__ALIGNED_MALLOC)
# posix_memalign (needs _POSIX_C_SOURCE >= 200112L on Linux)
set(CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=200112L)
check_symbol_exists(posix_memalign stdlib.h OPJ_HAVE_POSIX_MEMALIGN)
unset(CMAKE_REQUIRED_DEFINITIONS)
# memalign (obsolete)
check_symbol_exists(memalign malloc.h OPJ_HAVE_MEMALIGN)

add_subdirectory(src/lib)

configure_file(
 "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/opj_config.h.cmake.in"
 "${CMAKE_CURRENT_BINARY_DIR}/src/lib/openjp2/opj_config.h"
 @ONLY)

configure_file(
 "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/opj_config_private.h.cmake.in"
 "${CMAKE_CURRENT_BINARY_DIR}/src/lib/openjp2/opj_config_private.h"
 @ONLY)

