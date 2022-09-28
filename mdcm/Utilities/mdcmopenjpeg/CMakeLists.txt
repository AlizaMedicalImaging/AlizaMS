if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0042 NEW)
endif()

set(OPENJPEG_LIBRARY_NAME mdcmopenjp2)

project(${OPENJPEG_NAMESPACE} C)

set(BUILD_SHARED_LIBS OFF)

include_regular_expression("^.*$")

# OPENJPEG version number, useful for packaging and doxygen doc:
set(OPENJPEG_VERSION_MAJOR 2)
set(OPENJPEG_VERSION_MINOR 3)
set(OPENJPEG_VERSION_BUILD 1)
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

# --------------------------------------------------------------------------
# Path to additional CMake modules
set(CMAKE_MODULE_PATH
    ${${OPENJPEG_NAMESPACE}_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH})

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

set(MANGLE_PREFIX ${MDCM_OPENJPEG_LIBRARIES})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/openjpeg_mangle.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/openjpeg_mangle.h
               @ONLY)

# --------------------------------------------------------------------------
set(MANGLE_PREFIX ${MDCM_OPENJPEG_LIBRARIES})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/openjpeg_mangle.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/openjpeg_mangle.h
               @ONLY)

# --------------------------------------------------------------------------
# On Visual Studio 8 MS deprecated C. This removes all 1.276E1265 security
# warnings
if(WIN32)
  if(NOT BORLAND)
    if(NOT CYGWIN)
      if(NOT MINGW)
        if(NOT ITK_ENABLE_VISUAL_STUDIO_DEPRECATED_C_WARNINGS)
          add_definitions(
            -D_CRT_FAR_MAPPINGS_NO_DEPRECATE
            -D_CRT_IS_WCTYPE_NO_DEPRECATE
            -D_CRT_MANAGED_FP_NO_DEPRECATE
            -D_CRT_NONSTDC_NO_DEPRECATE
            -D_CRT_SECURE_NO_DEPRECATE
            -D_CRT_SECURE_NO_DEPRECATE_GLOBALS
            -D_CRT_SETERRORMODE_BEEP_SLEEP_NO_DEPRECATE
            -D_CRT_TIME_FUNCTIONS_NO_DEPRECATE
            -D_CRT_VCCLRIT_NO_DEPRECATE
            -D_SCL_SECURE_NO_DEPRECATE
            )
        endif()
      endif()
    endif()
  endif()
endif()

string(TOLOWER ${PROJECT_NAME} projectname)
set(OPENJPEG_INSTALL_SUBDIR "${projectname}-${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}")

if(NOT OPENJPEG_INSTALL_BIN_DIR)
  set(OPENJPEG_INSTALL_BIN_DIR "bin")
endif()

if(NOT OPENJPEG_INSTALL_LIB_DIR)
  set(OPENJPEG_INSTALL_LIB_DIR "lib")
endif()

if(NOT OPENJPEG_INSTALL_SHARE_DIR)
  set(OPENJPEG_INSTALL_SHARE_DIR "share")
endif()

if(NOT OPENJPEG_INSTALL_DATA_DIR)
  set(OPENJPEG_INSTALL_DATA_DIR "${OPENJPEG_INSTALL_SHARE_DIR}/${OPENJPEG_INSTALL_SUBDIR}")
endif()

if(NOT OPENJPEG_INSTALL_INCLUDE_DIR)
  set(OPENJPEG_INSTALL_INCLUDE_DIR "include/${OPENJPEG_INSTALL_SUBDIR}")
endif()

if(NOT OPENJPEG_INSTALL_JNI_DIR)
  if(WIN32)
    set(OPENJPEG_INSTALL_JNI_DIR ${OPENJPEG_INSTALL_BIN_DIR})
  else()
    set(OPENJPEG_INSTALL_JNI_DIR ${OPENJPEG_INSTALL_LIB_DIR})
  endif()
endif()

if(NOT OPENJPEG_INSTALL_PACKAGE_DIR)
  # We could install *.cmake files in share/ however those files contains
  # hardcoded path to libraries on a multi-arch system (fedora/debian) those
  # path will be different (lib/i386-linux-gnu vs lib/x86_64-linux-gnu)
  set(OPENJPEG_INSTALL_PACKAGE_DIR "${OPENJPEG_INSTALL_LIB_DIR}/${OPENJPEG_INSTALL_SUBDIR}")
endif()

if (APPLE)
	list(APPEND OPENJPEG_LIBRARY_PROPERTIES INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${OPENJPEG_INSTALL_LIB_DIR}")
	option(OPJ_USE_DSYMUTIL "Call dsymutil on binaries after build." OFF)
endif()

#-----------------------------------------------------------------------------
# Big endian test:
include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(OPJ_BIG_ENDIAN)

#-----------------------------------------------------------------------------
# Setup file for setting custom ctest vars
configure_file(
  ${${OPENJPEG_NAMESPACE}_SOURCE_DIR}/cmake/CTestCustom.cmake.in
  ${${OPENJPEG_NAMESPACE}_BINARY_DIR}/CTestCustom.cmake
  @ONLY)

#-----------------------------------------------------------------------------
# OpenJPEG build configuration options.
#option(BUILD_SHARED_LIBS "Build OpenJPEG shared library and link executables against it." ON)
set(EXECUTABLE_OUTPUT_PATH ${OPENJPEG_BINARY_DIR}/bin CACHE PATH "Single output directory for building all executables.")
set(LIBRARY_OUTPUT_PATH ${OPENJPEG_BINARY_DIR}/bin CACHE PATH "Single output directory for building all libraries.")
mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

# Compiler specific flags:
if(CMAKE_COMPILER_IS_GNUCC)
  # For all builds, make sure openjpeg is std99 compliant:
  # set(CMAKE_C_FLAGS "-Wall -std=c99 ${CMAKE_C_FLAGS}") # FIXME: this setting prevented us from setting a coverage build.
  # Do not use ffast-math for all build, it would produce incorrect results, only set for release:
  set(OPENJPEG_LIBRARY_COMPILE_OPTIONS ${OPENJPEG_LIBRARY_COMPILE_OPTIONS} "$<$<CONFIG:Release>:-ffast-math>")
endif()

#-----------------------------------------------------------------------------
# opj_config.h generation (1/2)

# Check if some include files are provided by the system
include(EnsureFileInclude)
# These files are mandatory
ensure_file_include("string.h" HAVE_STRING_H YES)
ensure_file_include("memory.h" HAVE_MEMORY_H YES)
ensure_file_include("stdlib.h" HAVE_STDLIB_H YES)
ensure_file_include("stdio.h"  HAVE_STDIO_H  YES)
ensure_file_include("math.h"   HAVE_MATH_H   YES)
ensure_file_include("float.h"  HAVE_FLOAT_H  YES)
ensure_file_include("time.h"   HAVE_TIME_H   YES)
ensure_file_include("stdarg.h" HAVE_STDARG_H YES)
ensure_file_include("ctype.h"  HAVE_CTYPE_H  YES)
ensure_file_include("assert.h" HAVE_ASSERT_H YES)

# For the following files, we provide an alternative, they are not mandatory
ensure_file_include("stdint.h"   OPJ_HAVE_STDINT_H   NO)
ensure_file_include("inttypes.h" OPJ_HAVE_INTTYPES_H NO)

# why check this one ? for openjpip ?
include (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE("strings.h"   HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("sys/stat.h"  HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h"    HAVE_UNISTD_H)

# Enable Large file support
include(TestLargeFiles)
OPJ_TEST_LARGE_FILES(OPJ_HAVE_LARGEFILES)

# Allocating Aligned Memory Blocks
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
#-----------------------------------------------------------------------------
add_subdirectory(src/lib)

#-----------------------------------------------------------------------------
# opj_config.h generation (2/2)
configure_file(
 ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/opj_config.h.cmake.in
 ${CMAKE_CURRENT_BINARY_DIR}/src/lib/openjp2/opj_config.h
 @ONLY)

 configure_file(
 ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/openjp2/opj_config_private.h.cmake.in
 ${CMAKE_CURRENT_BINARY_DIR}/src/lib/openjp2/opj_config_private.h
 @ONLY)


