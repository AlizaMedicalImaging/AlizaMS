####################################################################
#
# Aliza MS
#
####################################################################

cmake_minimum_required(VERSION 3.11.0)

####################################################################
#
#
#
#

# https://cmake.org/cmake/help/v3.0/policy/CMP0003.html
# Libraries linked via full path no longer produce linker search paths
CMAKE_POLICY(SET CMP0003 NEW)

# https://cmake.org/cmake/help/v3.0/policy/CMP0022.html
# Link
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()

# https://cmake.org/cmake/help/v3.0/policy/CMP0025.html
# AppleClang
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

# https://cmake.org/cmake/help/v3.6/policy/CMP0026.html
# Disallow LOCATION property
if(POLICY CMP0026)
  cmake_policy(SET CMP0026 NEW)
endif()

# https://cmake.org/cmake/help/v3.0/policy/CMP0042.html
# MACOSX_RPATH is enabled by default
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

# https://cmake.org/cmake/help/v3.3/policy/CMP0063.html
# Honor visibility properties for all target types.
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

#https://cmake.org/cmake/help/v3.19/policy/CMP0072.html
#FindOpenGL prefers GLVND by default when available.
#https://cmake.org/cmake/help/v3.19/module/FindOpenGL.html
if (POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()
#
#
#
#
####################################################################

project(ALIZAMS)

####################################################################
#
#
# Options
#
#
#
if(APPLE)
  set(ALIZAMS_OSX_BUNDLE_VERSION "1.8.4")
endif()

# Qt major version, required.
set(ALIZA_QT_VERSION "QT_VERSION_NOT_SELECTED" CACHE STRING "Qt major version")
set_property(CACHE ALIZA_QT_VERSION PROPERTY STRINGS "QT_VERSION_NOT_SELECTED" "4" "5" "6")

# C++ standard "Not set" is usually safe and recommended, except e.g.:
#   - C++11 is required for CCG 4.8
#   - C++14 may be required to avoid warnings on Debian 10 with distro's ITK 4.12
#   - testing
#   - etc.
# There is no guarantee that the option will work,
# check "__cplusplus" (s. main.cpp PRINT_HOST_INFO define).
set(ALIZA_CXX_STANDARD "Not set" CACHE STRING "Select C++ standard, usually not requied")
set_property(CACHE ALIZA_CXX_STANDARD PROPERTY STRINGS "Not set" "11" "14" "17" "20")

# Force disable SSE/SSE2.
option(ALIZA_DISABLE_SIMDMATH "Disable SSE/SSE2" OFF)

if(NOT APPLE)
  # Use OpenGL Core profile with Qt5 (with Qt6 always Core profile).
  # With Qt4 not defined, may work, not tested.
  option(ALIZA_QT5_COREGL "Use OpenGL Core profile with Qt5" OFF)
endif()

if(UNIX AND NOT APPLE)
  # Force xcb platform on Linux.
  # Another platform still can be set with an argument, e.g. "alizams -platform vnc".
  option(ALIZA_QT5_6_PLATFORM_XCB "Force QT_QPA_PLATFORM=xcb (only Qt5 and Qt6)" ON)
  option(ALIZA_QT4_SYSTEM_GLEW "Use system GLEW (only Qt4)" OFF)
  option(ALIZA_USE_SYSTEM_BULLET "Use system Bullet" OFF)
  option(ALIZA_USE_SYSTEM_LCMS2 "Use system lcms2" OFF)
endif()

# Only for build to be shipped on media.
option(ALIZA_MEDIASTORAGE_MODE "Build for media storage" OFF)
mark_as_advanced(ALIZA_MEDIASTORAGE_MODE)

# Temporary
set(TMP_USE_53_SPATIAL_ENUMS TRUE)

#
#
# MDCM
#
#

if(WIN32)
  option(MDCM_USE_WIN32_UNC "Unicode, long file path (Windows)" ON)
endif()

option(MDCM_USE_SYSTEM_ZLIB "Use system zlib" OFF)

if(UNIX AND NOT APPLE)
  option(MDCM_USE_SYSTEM_UUID "Use system uuid" OFF)
endif()

option(MDCM_USE_SYSTEM_OPENJPEG "Use system OpenJPEG" OFF)

option(MDCM_USE_SYSTEM_CHARLS "Use system CharLS" OFF)

# Currently not used.
if(FALSE)
  # OpenSSL is currently unused in AlizaMS.
  option(MDCM_USE_SYSTEM_OPENSSL "Use system OpenSSL" OFF)
  mark_as_advanced(MDCM_USE_SYSTEM_OPENSSL)
  
  # JPEG library released by the Standford PVRG group,
  # build (or use system) binary to convert to raw tmp file.
  option(MDCM_USE_PVRG "Use PVRG" OFF)
  mark_as_advanced(MDCM_USE_PVRG)
  if(MDCM_USE_PVRG)
    option(MDCM_USE_SYSTEM_PVRG "Use system PVRG" OFF)
    mark_as_advanced(MDCM_USE_SYSTEM_PVRG)
    if(MDCM_USE_SYSTEM_PVRG)
      find_package(PVRGJPEG REQUIRED)
    endif()
  endif()
endif()
#
#
#
#
#
#
####################################################################

message(STATUS "CPU: " ${CMAKE_SYSTEM_PROCESSOR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for executables")
  mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
endif()
if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for libraries")
  mark_as_advanced(LIBRARY_OUTPUT_PATH)
endif()

set(tmp0_build_type "None")
if(CMAKE_BUILD_TYPE)
  set(tmp0_build_type "${CMAKE_BUILD_TYPE}")
endif()
string(TOUPPER ${tmp0_build_type} tmp1_build_type)
if(${tmp1_build_type} STREQUAL "DEBUG")
  add_definitions(-D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

# Required for x86 32 bit GCC build only.
if(NOT WIN32)
  if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 8))
    if(NOT ALIZA_DISABLE_SIMDMATH)
      if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686")
        set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " -msse2")
        set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -msse2")
      endif()
    endif()
  endif()
endif()
message(STATUS "CXX flags: " ${CMAKE_CXX_FLAGS})

if(ALIZA_QT_VERSION STREQUAL "4")
  set(USE_QT_V_4 TRUE)
  message(STATUS "Using Qt4")
elseif(ALIZA_QT_VERSION STREQUAL "5")
  set(USE_QT_V_5 TRUE)
  message(STATUS "Using Qt5")
elseif(ALIZA_QT_VERSION STREQUAL "6")
  set(USE_QT_V_6 TRUE)
  message(STATUS "Using Qt6")
else()
  message(FATAL_ERROR "Can not determine Qt version to use")
endif()

if(ALIZA_CXX_STANDARD STREQUAL "Not set")
else()
  if(ALIZA_CXX_STANDARD STREQUAL "11")
    set(CMAKE_CXX_STANDARD 11)
    message(STATUS "Tried to set C++11")
  elseif(ALIZA_CXX_STANDARD STREQUAL "14")
    set(CMAKE_CXX_STANDARD 14)
    message(STATUS "Tried to set C++14")
  elseif(ALIZA_CXX_STANDARD STREQUAL "17")
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "Tried to set C++17")
  elseif(ALIZA_CXX_STANDARD STREQUAL "20")
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "Tried to set C++20")
  endif()
  if(UNIX AND NOT APPLE)
    #
    # To set GCC e.g. -std=c++11 instead of -std=gnu++11,
    # not 100% sure about that option.
    #
    set(CMAKE_CXX_EXTENSIONS OFF)
  endif()
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(UNIX AND NOT APPLE)
  if(ALIZA_QT5_6_PLATFORM_XCB)
    add_definitions(-DFORCE_PLATFORM_XCB)
  endif()
endif()

if(ALIZA_DISABLE_SIMDMATH)
  add_definitions(-DDISABLE_SIMDMATH)
else()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    add_definitions(-DDISABLE_SIMDMATH)
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    add_definitions(-DDISABLE_SIMDMATH)
  endif()
endif()

if(WIN32)
  if(MDCM_USE_WIN32_UNC)
    add_definitions(-DMDCM_WIN32_UNC)
  endif()
endif()

set(MDCM_LJPEG_LIBRARIES mdcmjpeg8 mdcmjpeg12 mdcmjpeg16)

if(MDCM_USE_SYSTEM_CHARLS)
  find_package(CharLS REQUIRED)
  set(MDCM_CHARLS_LIBRARIES ${CHARLS_LIBRARIES})
  if(CHARLS_LOW)
    add_definitions(-DMDCM_CHARLS_LOW)
  endif()
  # TODO update CharLS to be ready for future v3
  add_definitions(-DCHARLS_NO_DEPRECATED_WARNING)
else()
  set(MDCM_CHARLS_LIBRARIES mdcmcharls)
endif()

if(MDCM_USE_SYSTEM_OPENJPEG)
  find_package(OpenJPEG 2.0.0 REQUIRED)
  set(MDCM_OPENJPEG_LIBRARIES ${OPENJPEG_LIBRARIES})
else()
  set(MDCM_OPENJPEG_LIBRARIES mdcmopenjp2)
endif()

if(MDCM_USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIR})
  set(MDCM_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
else()
  set(MDCM_ZLIB_LIBRARIES "mdcmzlib")
endif()

if(UNIX)
  if(MDCM_USE_SYSTEM_UUID)
    find_package(UUID REQUIRED)
    set(MDCM_UUID_LIBRARIES ${UUID_LIBRARIES})
  else()
    set(MDCM_UUID_LIBRARIES "mdcmuuid")
  endif()
endif()

set(MDCM_LIBRARIES
  ${MDCM_LJPEG_LIBRARIES}
  ${MDCM_OPENJPEG_LIBRARIES}
  ${MDCM_CHARLS_LIBRARIES}
  ${MDCM_ZLIB_LIBRARIES})
if(NOT WIN32)
  set(MDCM_LIBRARIES ${MDCM_LIBRARIES} ${MDCM_UUID_LIBRARIES})
endif()
message(STATUS "MDCM: ${MDCM_LIBRARIES}")

include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(MDCM_WORDS_BIGENDIAN)

include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake)

#FIXME
macro(CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES("${UUID_INCLUDES};${FILE}" ${VARIABLE})
  if(${VARIABLE})
    set(UUID_INCLUDES ${UUID_INCLUDES} ${FILE})
  endif()
endmacro()
#

set(MDCM_MAJOR_VERSION 1)
set(MDCM_MINOR_VERSION 2)
set(MDCM_BUILD_VERSION 1)
set(MDCM_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}.${MDCM_BUILD_VERSION}")
set(MDCM_API_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}")

CHECK_INCLUDE_FILE_CONCAT("sys/time.h" MDCM_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("byteswap.h" MDCM_HAVE_BYTESWAP_FILE_H)
# s. GDCM bug #324
if(MDCM_HAVE_BYTESWAP_FILE_H)
  include(CheckCXXSourceCompiles)
  CHECK_CXX_SOURCE_COMPILES("\#include <byteswap.h>\nint main(){return bswap_32(42);}" MDCM_HAVE_BYTESWAP_H)
endif()
CHECK_INCLUDE_FILE("rpc.h" MDCM_HAVE_RPC_H)
CHECK_INCLUDE_FILE("langinfo.h" MDCM_HAVE_LANGINFO_H)

include(CheckFunctionExists)
include(CheckSymbolExists)

CHECK_SYMBOL_EXISTS(strcasecmp "strings.h" MDCM_HAVE_STRCASECMP)
CHECK_SYMBOL_EXISTS(strncasecmp "strings.h" MDCM_HAVE_STRNCASECMP)
CHECK_SYMBOL_EXISTS(snprintf "stdio.h" MDCM_HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS(strptime MDCM_HAVE_STRPTIME)
CHECK_FUNCTION_EXISTS(_stricmp MDCM_HAVE__STRICMP)
CHECK_FUNCTION_EXISTS(_strnicmp MDCM_HAVE__STRNICMP)
CHECK_FUNCTION_EXISTS(_snprintf MDCM_HAVE__SNPRINTF)
CHECK_FUNCTION_EXISTS(gettimeofday MDCM_HAVE_GETTIMEOFDAY)
CHECK_INCLUDE_FILE("stdint.h" MDCM_HAVE_STDINT_H)
if(UNIX)
  CHECK_INCLUDE_FILE("inttypes.h" MDCM_HAVE_INTTYPES_H)
endif()

include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("int main(){const char *f =__FUNCTION__;return 0;}" MDCM_CXX_HAS_FUNCTION)
CHECK_CXX_SOURCE_COMPILES("\#include <fstream>\nint main(){const wchar_t fn[10]={};std::ifstream is(fn);return 0;}" MDCM_HAVE_WCHAR_IFSTREAM)

if(MDCM_USE_SYSTEM_OPENSSL)
  set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS}) #FIXME
  CHECK_CXX_SOURCE_COMPILES("\#include <openssl/cms.h>\nint main(){CMS_add0_recipient_password(0,0,0,0,0,0,0);return 0;}" MDCM_HAVE_CMS_RECIPIENT_PASSWORD)
endif()

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common"
  "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/Common"
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary"
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition"
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat"
  "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities"
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities")

if(MDCM_USE_SYSTEM_CHARLS)
  include_directories(${CHARLS_INCLUDE_DIRS})
else()
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmcharls")
endif()

if(MDCM_USE_SYSTEM_OPENJPEG)
  include_directories(${OPENJPEG_INCLUDE_DIRS})
else()
  include_directories(BEFORE
    "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmopenjpeg"
    "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmopenjpeg/src/lib/openjp2"
    "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmopenjpeg/src/lib/openjp2")
endif()

if(NOT MDCM_USE_SYSTEM_ZLIB)
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmzlib")
  include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmzlib")
endif()

if(MDCM_USE_SYSTEM_UUID)
  include_directories(${UUID_INCLUDE_DIR})
  set(MDCMUUID ${UUID_LIBRARIES})
else()
  include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmuuid")
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmuuid")
  set(MDCMUUID mdcmuuid)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/Common/mdcmConfigure.h")

#
#
#
#
#
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities)
#
#
#
#
#

set(MDCM_COMMON_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmVersion.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmRegion.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmBoxRegion.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmEvent.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmMD5.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmBase64.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSHA1.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmCryptoFactory.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmASN1.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSubject.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmFilename.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmFilenameGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSwapCode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSystem.cxx)

if(WIN32)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmCAPICryptographicMessageSyntax.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmCAPICryptoFactory.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmOpenSSLP7CryptographicMessageSyntax.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL AND MDCM_HAVE_CMS_RECIPIENT_PASSWORD)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmOpenSSLCryptoFactory.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmOpenSSLCryptographicMessageSyntax.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(MDCM_DICT_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmDicts.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmDefaultDicts.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmPrivateDefaultDicts.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmGlobal.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmUIDs.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmCSAHeaderDefaultDicts.cxx)

set(MDCM_DSED_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmByteValue.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmDataSet.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmExplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmFile.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmFileMetaInformation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmFragment.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmImplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmMediaStorage.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmPrivateTag.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmSequenceOfFragments.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmSequenceOfItems.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmValue.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmTag.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmTagToVR.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmCodeString.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmByteSwapFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmUNExplicitImplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmCSAHeader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmMrProtocol.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmTransferSyntax.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmVM.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmPreamble.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmUNExplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmCP246ExplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmExplicitImplicitDataElement.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR16ExplicitDataElement.cxx)

# Code for segments and surfaces is currently unused.
set(MDCM_MSFF_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmFileExplicitFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageFragmentSplitter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSimpleSubjectWatcher.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmBitmap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmRescaler.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageToImageFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmBitmapToBitmapFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapToPixmapFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangeTransferSyntax.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageApplyLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmApplySupplementalLUT.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmFileDecompressLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmOrientation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmDataSetHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangePlanarConfiguration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmDirectionCosines.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSplitMosaicFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmUIDGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmUUIDGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmScanner.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImage.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG12Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmRLECodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG16Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG8Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPVRGCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmRAWCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmEncapsulatedRAWCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmOverlay.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmCurve.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPhotometricInterpretation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixelFormat.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentedPaletteColorLookupTable.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegment.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurface.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmMeshPrimitive.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentWriter.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceWriter.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentReader.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceReader.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceHelper.cxx
#  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmrle/rle.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmrle/info.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmrle/io.cxx)

if(NOT MDCM_USE_SYSTEM_OPENJPEG)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx PROPERTIES COMPILE_DEFINITIONS OPJ_STATIC)
endif()

if(NOT MDCM_USE_SYSTEM_CHARLS)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx PROPERTIES COMPILE_DEFINITIONS CHARLS_STATIC)
else()
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx PROPERTIES COMPILE_DEFINITIONS CHARLS_DLL)
endif()

#
#
#
#
#

if(USE_QT_V_5 OR USE_QT_V_6)
  if(WIN32)
# https://cmake.org/cmake/help/v3.0/policy/CMP0020.html
# Automatically link Qt executables to qtmain target on Windows.
    CMAKE_POLICY(SET CMP0020 NEW)
  endif()
endif()

find_package(OpenGL REQUIRED)

if(APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

find_package(ITK COMPONENTS
  ITKCommon
  ITKImageFilterBase
  ITKImageFunction
  ITKImageGrid
  ITKImageIntensity
  ITKSpatialObjects
  ITKStatistics
  ITKTransform)
include(${ITK_USE_FILE})
message(STATUS "ITK: ${ITK_LIBRARIES}")

if(USE_QT_V_6)
  find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    OpenGLWidgets
    PrintSupport
    Svg
    Core5Compat REQUIRED)
  if(NOT ALIZA_MEDIASTORAGE_MODE)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
  endif()
  include_directories(${Qt6Core5Compat_INCLUDE_DIRS})
elseif(USE_QT_V_5)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5PrintSupport REQUIRED)
  find_package(Qt5Svg REQUIRED)
  if(NOT ALIZA_MEDIASTORAGE_MODE)
    find_package(Qt5Sql REQUIRED)
  endif()
elseif(USE_QT_V_4)
  find_package(Qt4 REQUIRED)
  set(QT_USE_QTMAIN TRUE)
  set(QT_USE_QTGUI TRUE)
  set(QT_USE_QTOPENGL TRUE)
  set(QT_USE_QTSVG TRUE)
  if(NOT ALIZA_MEDIASTORAGE_MODE)
    set(QT_USE_QTSQL TRUE)
  endif()
  include(${QT_USE_FILE})
  #add_definitions(${QT_DEFINITIONS})
  #message(STATUS "QT_DEFINITIONS: ${QT_DEFINITIONS}")
  if(ALIZA_QT4_SYSTEM_GLEW)
    set(LOCAL_GLEW_PATH "")
  else()
    set(LOCAL_GLEW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CG/glew")
    add_definitions(-DGLEW_STATIC)
  endif()
  set(USE_QT_V_4 TRUE)
else()
  message(FATAL_ERROR "Can not determine Qt version to use")
endif()

if(NOT ALIZA_MEDIASTORAGE_MODE)
  add_definitions(-DUSE_WORKSTATION_MODE)
endif()

if(WIN32)
  if(MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400)
    add_definitions(
     -D_CRT_SECURE_NO_DEPRECATE
     -D_CRT_NONSTDC_NO_DEPRECATE
     -D_CRT_SECURE_NO_WARNINGS
     -D_SCL_SECURE_NO_DEPRECATE
     -D_SCL_SECURE_NO_WARNINGS)
  endif()
endif()

set(ALIZAMS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/common/structures.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

if(NOT ALIZA_USE_SYSTEM_BULLET)
  include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/b)
  SET(BULLET_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btAlignedAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btConvexHull.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btConvexHullComputer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btGeometryUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btPolarDecomposition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btQuickprof.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btReducedVector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btSerializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btSerializer64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btThreads.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/btVector3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/TaskScheduler/btTaskScheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/TaskScheduler/btThreadSupportPosix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/LinearMath/TaskScheduler/btThreadSupportWin32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btDbvt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCollisionObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCollisionWorld.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCollisionWorldImporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btGhostObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btManifoldResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/btUnionFind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btBoxShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btBox2dShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btCapsuleShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btCollisionShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btCompoundShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConcaveShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConeShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexHullShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexInternalShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvex2dShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btCylinderShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btEmptyShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btMiniSDF.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btMultiSphereShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btOptimizedBvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btSdfCollisionShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btShapeHull.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btSphereShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTetrahedronShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleCallback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/CollisionShapes/btUniformScalingShape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/b/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${BULLET_SRCS})
else()
  include_directories("/usr/include/bullet")
endif()

#
#
#
#
#
if(ALIZA_USE_SYSTEM_LCMS2)
  find_package(LCMS2 REQUIRED)
  add_definitions(-DALIZA_USE_SYSTEM_LCMS2)
  message(STATUS "LCMS2: " ${LCMS2_LIBRARIES})
else()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alizalcms)
  message(STATUS "LCMS2: alizalcms2")
endif()
#
#
#
#
#

set(ALIZAMS_SRCS ${ALIZAMS_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/common/commonutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/contourutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/colorspace/colorspace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/codecutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/ultrasoundregionutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/dicomutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/prconfigutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/splituihgridfilter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/spectroscopyutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dicom/srutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CG/camera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CG/testgl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/mainwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/updateqtcommand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/iconutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/histogramgen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/settingswidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/qxtspanslider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imagesbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/animwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox2D.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/sliderwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zrangewidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/lutwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/handleitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/levelitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/rectitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicsutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicspathitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicsview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studygraphicswidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studygraphicsview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicswidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/histogramview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zoomwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/findrefdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imageinfodialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.cpp
  #${CMAKE_CURRENT_SOURCE_DIR}/GUI/tabledialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/matrixbutton.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyframewidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyviewwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/sqtree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/browserwidget2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/helpwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/anonymazerwidget2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aliza.cpp)

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.cpp)
endif()

if(USE_QT_V_4 AND NOT ALIZA_QT4_SYSTEM_GLEW)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} "${LOCAL_GLEW_PATH}/src/glew.c")
endif()

if(WIN32)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/alizams.rc)
endif()

set(ALIZAMS_QRCS ${CMAKE_CURRENT_SOURCE_DIR}/alizams.qrc)

set(ALIZAMS_MOC_HRDS
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/mainwindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/sqtree.h
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/browserwidget2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/helpwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/anonymazerwidget2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/findrefdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imageinfodialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/settingswidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/qxtspanslider_p.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/qxtspanslider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imagesbox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/animwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox2D.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/sliderwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zrangewidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/lutwidget.h
  #${CMAKE_CURRENT_SOURCE_DIR}/GUI/tabledialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/matrixbutton.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyframewidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyviewwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicsview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicswidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studygraphicsview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studygraphicswidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/histogramview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zoomwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aliza.h)

if(APPLE)
  if(USE_QT_V_6)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5-core.h)
    add_definitions(-DALIZA_GL_3_2_CORE)
  else()
    message(FATAL_ERROR "Only Qt6 is supported with macOS")
  endif()
elseif(WIN32)
  if(USE_QT_V_6)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5-core.h)
    add_definitions(-DALIZA_GL_3_2_CORE)
  elseif(USE_QT_V_5)
    if(ALIZA_QT5_COREGL)
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5-core.h)
      add_definitions(-DALIZA_GL_3_2_CORE)
    else()
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5.h)
    endif()
  elseif(USE_QT_V_4)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt4.h)
  else()
    message(FATAL_ERROR "Can not determine Qt version to use")
  endif()
else()
  if(USE_QT_V_6)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5-core.h)
    add_definitions(-DALIZA_GL_3_2_CORE)
  elseif(USE_QT_V_5)
    if(ALIZA_QT5_COREGL)
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5-core.h)
      add_definitions(-DALIZA_GL_3_2_CORE)
    else()
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5.h)
    endif()
  elseif(USE_QT_V_4)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt4.h)
  else()
    message(FATAL_ERROR "Can not determine Qt version to use")
  endif()
endif()

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.h)
endif()

set(ALIZAMS_UIS
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/settingswidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/mainwindow.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/sqtree.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/browserwidget2.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/helpwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/anonymazerwidget2.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/findrefdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imagesbox.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/animwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox2D.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/sliderwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.ui
  #${CMAKE_CURRENT_SOURCE_DIR}/GUI/tabledialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyframewidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/studyviewwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imageinfodialog.ui)

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_UIS ${ALIZAMS_UIS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.ui)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/common"
  "${CMAKE_CURRENT_SOURCE_DIR}/GUI"
  "${CMAKE_CURRENT_SOURCE_DIR}/dicom"
  ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  link_directories(/usr/local/lib)
endif()

set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${MDCM_COMMON_SRCS} ${MDCM_DICT_SRCS} ${MDCM_DSED_SRCS} ${MDCM_MSFF_SRCS})

set(ALIZAMS_LINK_LIBRARIES ${MDCM_LIBRARIES} ${ITK_LIBRARIES})

if(USE_QT_V_6)
  QT6_WRAP_CPP(ALIZAMS_MOC_SRCS ${ALIZAMS_MOC_HRDS})
  QT6_WRAP_UI(ALIZAMS_UI ${ALIZAMS_UIS})
  QT6_ADD_RESOURCES(ALIZAMS_QRCS_RCC ${ALIZAMS_QRCS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::PrintSupport
    Qt6::Svg
    Qt6::Core5Compat)
  if(NOT ALIZA_MEDIASTORAGE_MODE)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} Qt6::Sql)
  endif()
elseif(USE_QT_V_5)
  QT5_WRAP_CPP(ALIZAMS_MOC_SRCS ${ALIZAMS_MOC_HRDS})
  QT5_WRAP_UI(ALIZAMS_UI ${ALIZAMS_UIS})
  QT5_ADD_RESOURCES(ALIZAMS_QRCS_RCC ${ALIZAMS_QRCS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Svg)
  if(NOT ALIZA_MEDIASTORAGE_MODE)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} Qt5::Sql)
  endif()
elseif(USE_QT_V_4)
  QT4_WRAP_CPP(ALIZAMS_MOC_SRCS ${ALIZAMS_MOC_HRDS})
  QT4_WRAP_UI(ALIZAMS_UI ${ALIZAMS_UIS})
  QT4_ADD_RESOURCES(ALIZAMS_QRCS_RCC ${ALIZAMS_QRCS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${QT_LIBRARIES})
else()
  message(FATAL_ERROR "Can not determine Qt version to use")
endif()

if(ALIZA_USE_SYSTEM_BULLET)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} libLinearMath.so libBulletCollision.so)
endif()

if(ALIZA_USE_SYSTEM_LCMS2)
  include_directories(${LCMS2_INCLUDE_DIRS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${LCMS2_LIBRARIES})
else()
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} alizalcms2)
endif()

if(TMP_USE_53_SPATIAL_ENUMS)
  add_definitions(-DTMP_USE_53_SPATIAL_ENUMS)
endif()

link_directories("${CMAKE_CURRENT_BINARY_DIR}/bin")

if(WIN32)
  if(USE_QT_V_4)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} OpenGL::GL)
  endif()
  add_executable(alizams WIN32
    ${ALIZAMS_SRCS}
    ${ALIZAMS_MOC_SRCS}
    ${ALIZAMS_UIS}
    ${ALIZAMS_QRCS_RCC})
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES} crypt32 rpcrt4)
elseif(APPLE)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES})
  set(ALIZAMS_OSX_ICON_NAME "AlizaMS.icns")
  set(ALIZAMS_OSX_ICON ${CMAKE_CURRENT_SOURCE_DIR}/${ALIZAMS_OSX_ICON_NAME})
  set_source_files_properties(${ALIZAMS_OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  add_executable(AlizaMS MACOSX_BUNDLE
    ${ALIZAMS_SRCS}
    ${ALIZAMS_MOC_SRCS}
    ${ALIZAMS_UIS}
    ${ALIZAMS_QRCS_RCC}
    ${ALIZAMS_OSX_ICON})
  set_target_properties(AlizaMS PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "AlizaMS"
    MACOSX_BUNDLE_BUNDLE_VERSION ${ALIZAMS_OSX_BUNDLE_VERSION}
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.aliza.alizams"
    MACOSX_BUNDLE_ICON_FILE ${ALIZAMS_OSX_ICON_NAME}
    MACOSX_BUNDLE_COPYRIGHT "© 2015-2022 Aliza Medical Imaging")
  target_link_libraries(AlizaMS ${ALIZAMS_LINK_LIBRARIES})
else()
  if(USE_QT_V_4)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} OpenGL::GL)
    if(ALIZA_QT4_SYSTEM_GLEW)
      set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} GLEW)
    endif()
  endif()
  add_executable(alizams
    ${ALIZAMS_SRCS}
    ${ALIZAMS_MOC_SRCS}
    ${ALIZAMS_UIS}
    ${ALIZAMS_QRCS_RCC})
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES})
  install(TARGETS alizams RUNTIME DESTINATION "bin")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/icons" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/applications" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/metainfo" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/man" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/alizams" DESTINATION "share")
endif()

