####################################################################
#
# Aliza MS
#
####################################################################
cmake_minimum_required(VERSION 3.11.0)

#######################################################
#
#
#
#

# https://cmake.org/cmake/help/v3.0/policy/CMP0003.html
# Libraries linked via full path no longer produce linker search paths
CMAKE_POLICY(SET CMP0003 NEW)
if(USE_QT_V_5)
  if(WIN32)
# https://cmake.org/cmake/help/v3.0/policy/CMP0020.html
# Automatically link Qt executables to qtmain target on Windows
    CMAKE_POLICY(SET CMP0020 NEW)
  endif()
endif()

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
if(POLICY CMP0063 )
  cmake_policy(SET CMP0063 NEW)
endif()

#
#
#
#
#######################################################

project(ALIZAMS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for executables")
  mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
endif()
if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for libraries")
  mark_as_advanced(LIBRARY_OUTPUT_PATH)
endif()

option(USE_QT_V_5 "Use Qt5" ON)
option(USE_SYSTEM_GLEW_QT4 "Use system GLEW (only Qt4)" OFF)
option(USE_MEDIASTORAGE_MODE "Build for media storage" OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# TMP
# set "ON" to build with GCC 4.8
option(USE_FORCE_CXX11 "Set standard to C++11" OFF)
mark_as_advanced(USE_FORCE_CXX11)
if(USE_FORCE_CXX11)
  set(CMAKE_CXX_STANDARD 11)
endif()
#

# TMP
# set "ON" to workaround Debian 10 ITK-4.12 package VNL warning
option(USE_FORCE_CXX14 "Set standard to C++14" OFF)
mark_as_advanced(USE_FORCE_CXX14)
if(USE_FORCE_CXX14)
  set(CMAKE_CXX_STANDARD 14)
endif()
#

option(MDCM_USE_SYSTEM_ZLIB "Use system zlib" OFF)

#option(MDCM_USE_SYSTEM_OPENSSL "Use system OpenSSL" OFF)
#mark_as_advanced(MDCM_USE_SYSTEM_OPENSSL)

if(UNIX)
  option(MDCM_USE_SYSTEM_UUID "Use system uuid" OFF)
endif()

option(MDCM_USE_SYSTEM_OPENJPEG "Use system OpenJPEG" OFF)

option(MDCM_USE_SYSTEM_CHARLS "Use system CharLS" OFF)

# TODO try system, if possible
set(MDCM_LJPEG_LIBRARIES mdcmjpeg8 mdcmjpeg12 mdcmjpeg16)

if(MDCM_USE_SYSTEM_CHARLS)
  find_package(CharLS REQUIRED)
  set(MDCM_CHARLS_LIBRARIES ${CHARLS_LIBRARIES})
else()
  set(MDCM_CHARLS_LIBRARIES mdcmcharls)
endif()

if(MDCM_USE_SYSTEM_OPENJPEG)
  find_package(OpenJPEG 2.0.0 REQUIRED)
  set(MDCM_OPENJPEG_LIBRARIES ${OPENJPEG_LIBRARIES})
else()
  set(MDCM_OPENJPEG_LIBRARIES mdcmopenjp2)
endif()

# JPEG library released by the Standford PVRG group,
# build (or use system) binary to convert to raw tmp file
option(MDCM_USE_PVRG "Use PVRG, advanced" OFF)
mark_as_advanced(MDCM_USE_PVRG)
if(MDCM_USE_PVRG)
  option(MDCM_USE_SYSTEM_PVRG "Use system PVRG" OFF)
  mark_as_advanced(MDCM_USE_SYSTEM_PVRG)
  if(MDCM_USE_SYSTEM_PVRG)
    find_package(PVRGJPEG REQUIRED)
  endif()
endif()

#option(MDCM_USE_KAKADU "Use Kakadu, advanced" OFF)
#mark_as_advanced(MDCM_USE_KAKADU)
#if(MDCM_USE_KAKADU)
#  option(MDCM_USE_SYSTEM_KAKADU "Use system KAKADU " ON)
#  mark_as_advanced(MDCM_USE_SYSTEM_KAKADU)
#  if(MDCM_USE_SYSTEM_KAKADU)
#    find_package(KAKADU REQUIRED)
#  else()
#    message(FATAL_ERROR "Not Implemented")
#  endif()
#endif()

if(MDCM_USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIR})
  set(MDCM_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
else()
  set(MDCM_ZLIB_LIBRARIES "mdcmzlib")
endif()

if(NOT WIN32)
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
set(MDCM_MINOR_VERSION 0)
set(MDCM_BUILD_VERSION 28)
set(MDCM_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}.${MDCM_BUILD_VERSION}")
set(MDCM_API_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}")

option(MDCM_SUPPORT_BROKEN_IMPLEMENTATION "Handle broken DICOM" ON)
mark_as_advanced(MDCM_SUPPORT_BROKEN_IMPLEMENTATION)

CHECK_INCLUDE_FILE_CONCAT("sys/time.h" MDCM_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("byteswap.h" MDCM_HAVE_BYTESWAP_FILE_H)
# s. GDCM bug #324
if(MDCM_HAVE_BYTESWAP_FILE_H)
  include(CheckCXXSourceCompiles)
  CHECK_CXX_SOURCE_COMPILES("\#include <byteswap.h>\nint main(){return bswap_32(42);}" MDCM_HAVE_BYTESWAP_H)
endif()
CHECK_INCLUDE_FILE("rpc.h"      MDCM_HAVE_RPC_H)
CHECK_INCLUDE_FILE("langinfo.h" MDCM_HAVE_LANGINFO_H)

include(CheckFunctionExists)
include(CheckSymbolExists)
#FIXME
#set(CMAKE_REQUIRED_INCLUDES math.h)
#set(CMAKE_REQUIRED_LIBRARIES m)
CHECK_SYMBOL_EXISTS(nl_langinfo "langinfo.h" MDCM_HAVE_NL_LANGINFO)
#CHECK_FUNCTION_EXISTS(strcasecmp MDCM_HAVE_STRCASECMP)
CHECK_SYMBOL_EXISTS(strcasecmp "strings.h" MDCM_HAVE_STRCASECMP)
#CHECK_FUNCTION_EXISTS(strncasecmp MDCM_HAVE_STRNCASECMP)
CHECK_SYMBOL_EXISTS(strncasecmp "strings.h" MDCM_HAVE_STRNCASECMP)
#CHECK_FUNCTION_EXISTS(snprintf MDCM_HAVE_SNPRINTF)
CHECK_SYMBOL_EXISTS(snprintf "stdio.h" MDCM_HAVE_SNPRINTF)
#CHECK_SYMBOL_EXISTS(strptime "time.h" MDCM_HAVE_STRPTIME)
CHECK_FUNCTION_EXISTS(strptime MDCM_HAVE_STRPTIME)
CHECK_FUNCTION_EXISTS(_stricmp  MDCM_HAVE__STRICMP)
CHECK_FUNCTION_EXISTS(_strnicmp MDCM_HAVE__STRNICMP)
CHECK_FUNCTION_EXISTS(_snprintf MDCM_HAVE__SNPRINTF)
CHECK_FUNCTION_EXISTS(lround MDCM_HAVE_LROUND)
CHECK_FUNCTION_EXISTS(gettimeofday MDCM_HAVE_GETTIMEOFDAY)

#TODO remove
CHECK_INCLUDE_FILE("stdint.h" MDCM_HAVE_STDINT_H)
if(UNIX)
  CHECK_INCLUDE_FILE("inttypes.h" MDCM_HAVE_INTTYPES_H)
endif()
if(NOT MDCM_HAVE_STDINT_H)
  if(MSVC OR "x${CMAKE_CXX_COMPILER_ID}x" MATCHES "Intel")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/C99")
  endif()
endif()
#

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
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmcharls")
endif()

if(MDCM_USE_SYSTEM_OPENJPEG)
  include_directories(${OPENJPEG_INCLUDE_DIRS})
else()
  include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmopenjpeg"
    "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmopenjpeg/src/lib/openjp2"
    "${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmopenjpeg/src/lib/openjp2")
endif()

if(NOT MDCM_USE_SYSTEM_ZLIB)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmzlib")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmzlib")
endif()

if(MDCM_USE_SYSTEM_UUID)
  include_directories(${UUID_INCLUDE_DIR})
  set(MDCMUUID ${UUID_LIBRARIES})
else()
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/mdcm/Utilities/mdcmuuid")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmuuid")
  set(MDCMUUID mdcmuuid)
endif()

if(NOT MDCM_USE_SYSTEM_ZLIB)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities/mdcmzlib")
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
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmDummyValueGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmCryptoFactory.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmASN1.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSubject.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmDirectory.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmFilename.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmFilenameGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSwapCode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmSystem.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmTrace.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/Common/mdcmUnpacker12Bits.cxx)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmDictEntry.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmDefaultDicts.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmPrivateDefaultDicts.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmGlobal.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmGroupDict.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmDefaultGroupNames.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmUIDs.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataDictionary/mdcmCSAHeaderDefaultDicts.cxx)


#FIXME
# dual compilation (namespace mdcm_ns), all the following either directly
# reference code using MDCM_SUPPORT_BROKEN_IMPLEMENTATION or indirectly
set(MDCM_DSED2_SRCS
  mdcmByteValue.cxx
  mdcmDataElement.cxx
  mdcmDataSet.cxx
  mdcmExplicitDataElement.cxx
  mdcmFile.cxx # FileMeta is class member
  mdcmFileMetaInformation.cxx # subclass of DataSet
  mdcmFragment.cxx
  mdcmImplicitDataElement.cxx
  mdcmMediaStorage.cxx # SetFromModality takes a DataSet
  mdcmPrivateTag.cxx
  mdcmReader.cxx
  mdcmSequenceOfFragments.cxx
  mdcmSequenceOfItems.cxx
  mdcmValue.cxx) # friend decl

# need to prepare duplicate files to help cmake handle setting compile
# definitions (cmake cannot handle duplicate source file in same target).
# this trick allows us to avoid a static compilation which may not be portable
foreach(src ${MDCM_DSED2_SRCS})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/${src}
    ${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/strict_${src}
    COPYONLY)
  list(APPEND MDCM_DSED3_SRCS ${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/strict_${src})
  set_property(
    SOURCE ${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/strict_${src}
    PROPERTY COMPILE_DEFINITIONS "MDCM_OVERRIDE_BROKEN_IMPLEMENTATION" "mdcm_ns=mdcmstrict")
endforeach()

foreach(src ${MDCM_DSED2_SRCS})
  list(APPEND MDCM_DSED4_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/${src})
endforeach()

# the following source code do not need a duplicate compilation to handle
# MDCM_SUPPORT_BROKEN_IMPLEMENTATION
set(MDCM_DSED_SRCS
  ${MDCM_DSED4_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmReader.strict.cxx # hook to call mdcmstrict API
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmTag.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmTagToVR.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmCodeString.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmFileSet.cxx
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
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR16ExplicitDataElement.cxx
  ${MDCM_DSED3_SRCS})

set(MDCM_MSFF_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmDICOMDIRGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmFileExplicitFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmFileDerivation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageFragmentSplitter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmTagPath.cxx
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
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSorter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmIPPSorter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSplitMosaicFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmUIDGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmUUIDGenerator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmScanner.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmStrictScanner.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmStringFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmValidate.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImage.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageConverter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmImageCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG12Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmRLECodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG16Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG8Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPVRGCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmKAKADUCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPNMCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPGXCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmRAWCodec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmOverlay.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmCurve.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPhotometricInterpretation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmPixelFormat.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentedPaletteColorLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmDirectoryHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegment.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurface.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmMeshPrimitive.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Source/MediaStorageAndFileFormat/mdcmSerieHelper.cxx
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

find_package(ITK COMPONENTS
  ITKCommon
  ITKImageFilterBase
  ITKImageFunction
  ITKImageGrid
  ITKImageIntensity
  ITKSpatialObjects
  ITKStatistics)
include(${ITK_USE_FILE})
message(STATUS "ITK: ${ITK_LIBRARIES}")

if(USE_QT_V_5)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5PrintSupport REQUIRED)
  find_package(Qt5Svg REQUIRED)
  if(NOT USE_MEDIASTORAGE_MODE)
    find_package(Qt5Sql REQUIRED)
  endif()
else()
  find_package(Qt4 REQUIRED)
  set(QT_USE_QTMAIN TRUE)
  set(QT_USE_QTGUI TRUE)
  set(QT_USE_QTOPENGL TRUE)
  set(QT_USE_QTSVG TRUE)
  if(NOT USE_MEDIASTORAGE_MODE)
    set(QT_USE_QTSQL TRUE)
  endif()
  include(${QT_USE_FILE})
  if(USE_SYSTEM_GLEW_QT4)
    set(LOCAL_GLEW_PATH "")
  else()
    set(LOCAL_GLEW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CG/glew")
    add_definitions(-DGLEW_STATIC)
  endif()
endif()

#required?
add_definitions(${QT_DEFINITIONS})
#message(STATUS "QT_DEFINITIONS: ${QT_DEFINITIONS}")

if(NOT USE_MEDIASTORAGE_MODE)
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/b)
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

set(ALIZAMS_SRCS ${ALIZAMS_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/common/commonutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/contourutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/colorspace/colorspace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/common/filepath.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/labelwidget.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicswidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/histogramview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zoomwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/findrefdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imageinfodialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/sqtree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/browserwidget2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aliza.cpp)

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.cpp)
endif()

if(NOT USE_QT_V_5 AND NOT USE_SYSTEM_GLEW_QT4)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/labelwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zrangewidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/lutwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicsview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/graphicswidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/histogramview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/zoomwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aliza.h)

if(USE_QT_V_5)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt5.h)
else()
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/CG/glwidget-qt4.h)
endif()

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.h)
endif()

set(ALIZAMS_UIS
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/settingswidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/mainwindow.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/aboutwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/sqtree.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/browser/browserwidget2.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/findrefdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imagesbox.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/animwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/toolbox2D.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/sliderwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/srwidget.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/GUI/imageinfodialog.ui)

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_UIS ${ALIZAMS_UIS} ${CMAKE_CURRENT_SOURCE_DIR}/browser/ctkdialog.ui)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/common"
  "${CMAKE_CURRENT_SOURCE_DIR}/GUI"
  "${CMAKE_CURRENT_SOURCE_DIR}/dicom"
  ${CMAKE_CURRENT_BINARY_DIR})

# Available defs: -DDISABLE_SIMDMATH to disable SSE2 math
if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  link_directories(/usr/local/lib)
endif()
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  add_definitions(-DDISABLE_SIMDMATH)
endif()

set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${MDCM_COMMON_SRCS} ${MDCM_DICT_SRCS} ${MDCM_DSED_SRCS} ${MDCM_MSFF_SRCS})

set(ALIZAMS_LINK_LIBRARIES ${MDCM_LIBRARIES} ${ITK_LIBRARIES})

if(USE_QT_V_5)
  QT5_WRAP_CPP(ALIZAMS_MOC_SRCS ${ALIZAMS_MOC_HRDS})
  QT5_WRAP_UI(ALIZAMS_UI ${ALIZAMS_UIS})
  QT5_ADD_RESOURCES(ALIZAMS_QRCS_RCC ${ALIZAMS_QRCS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Svg)
  if(NOT USE_MEDIASTORAGE_MODE)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} Qt5::Sql)
  endif()
else()
  QT4_WRAP_CPP(ALIZAMS_MOC_SRCS ${ALIZAMS_MOC_HRDS})
  QT4_WRAP_UI(ALIZAMS_UI ${ALIZAMS_UIS})
  QT4_ADD_RESOURCES(ALIZAMS_QRCS_RCC ${ALIZAMS_QRCS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${QT_LIBRARIES})
endif()

link_directories("bin")

if(WIN32)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} opengl32)
  if(NOT USE_QT_V_5 AND USE_SYSTEM_GLEW_QT4)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} glew32)
  endif()
  add_executable(alizams WIN32
    ${ALIZAMS_SRCS}
    ${ALIZAMS_MOC_SRCS}
    ${ALIZAMS_UIS}
    ${ALIZAMS_QRCS_RCC})
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES} crypt32 rpcrt4)
else()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} GLES)
  else()
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} GL)
  endif()
  if(NOT USE_QT_V_5 AND USE_SYSTEM_GLEW_QT4)
    set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} GLEW)
  endif()
  add_executable(alizams
    ${ALIZAMS_SRCS}
    ${ALIZAMS_MOC_SRCS}
    ${ALIZAMS_UIS}
    ${ALIZAMS_QRCS_RCC})
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES})
endif()

install(TARGETS alizams RUNTIME DESTINATION "bin")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/icons" DESTINATION "share")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/applications" DESTINATION "share")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/man" DESTINATION "share")
