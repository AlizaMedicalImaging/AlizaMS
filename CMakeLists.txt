####################################################################
#
# Aliza MS
#
####################################################################
cmake_minimum_required(VERSION 3.11.0)

project(ALIZAMS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

option(USE_QT_V_5 "Use Qt5" ON)
option(USE_SYSTEM_GLEW_QT4 "Use system GLEW (only Qt4)" OFF)
option(USE_MEDIASTORAGE_MODE "Build for media storage" OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# TMP
# set "ON" to workaround Debian 10 ITK-4.12 package VNL warning
option(USE_FORCE_CXX14 "Set standard to C++14" OFF)
mark_as_advanced(USE_FORCE_CXX14)
if(USE_FORCE_CXX14)
  set(CMAKE_CXX_STANDARD 14)
endif()
#

# TMP
# set "ON" to build with GCC 4.8
option(USE_FORCE_CXX11 "Set standard to C++11" OFF)
mark_as_advanced(USE_FORCE_CXX11)
if(USE_FORCE_CXX11)
  set(CMAKE_CXX_STANDARD 11)
endif()
#

CMAKE_POLICY(SET CMP0003 NEW)
if(USE_QT_V_5)
  if(WIN32)
    if(POLICY CMP0020)
      CMAKE_POLICY(SET CMP0020 NEW)
    endif()
  endif()
endif()

option(MDCM_USE_SYSTEM_ZLIB "Use system zlib" OFF)

#option(MDCM_USE_SYSTEM_OPENSSL "Use system OpenSSL" OFF)
#mark_as_advanced(MDCM_USE_SYSTEM_OPENSSL)

if(UNIX)
  option(MDCM_USE_SYSTEM_UUID "Use system uuid" OFF)
endif()

option(MDCM_USE_SYSTEM_OPENJPEG "Use system OpenJPEG" OFF)

option(MDCM_USE_SYSTEM_CHARLS "Use system CharLS" OFF)

#TODO
#option(MDCM_USE_SYSTEM_LJPEG "Use system JPEG (IJG)" OFF)
#mark_as_advanced(MDCM_USE_SYSTEM_LJPEG)
#if(MDCM_USE_SYSTEM_LJPEG)
#  find_package(LJPEG REQUIRED)
#  set(MDCM_LJPEG_LIBRARIES ${LJPEG_LIBRARIES})
#else()
  set(MDCM_LJPEG_LIBRARIES mdcmjpeg8 mdcmjpeg12 mdcmjpeg16)
#endif()

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

if(MDCM_USE_SYSTEM_UUID)
  find_package(UUID REQUIRED)
  set(MDCM_UUID_LIBRARIES ${UUID_LIBRARIES})
else()
  set(MDCM_UUID_LIBRARIES "mdcmuuid")
endif()

include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
TEST_BIG_ENDIAN(MDCM_WORDS_BIGENDIAN)
message(STATUS "TEST_BIG_ENDIAN=${MDCM_WORDS_BIGENDIAN}")

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

if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for executables")
  mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
endif()
if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "Output directory for libraries")
  mark_as_advanced(LIBRARY_OUTPUT_PATH)
endif()

set(MDCM_MAJOR_VERSION 1)
set(MDCM_MINOR_VERSION 0)
set(MDCM_BUILD_VERSION 28)
set(MDCM_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}.${MDCM_BUILD_VERSION}")
set(MDCM_API_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}")

option(MDCM_SUPPORT_BROKEN_IMPLEMENTATION "Handle broken DICOM" ON)
mark_as_advanced(MDCM_SUPPORT_BROKEN_IMPLEMENTATION)

CHECK_INCLUDE_FILE_CONCAT("sys/time.h" MDCM_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("winsock.h"  MDCM_HAVE_WINSOCK_H)
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
set(CMAKE_REQUIRED_INCLUDES math.h)
set(CMAKE_REQUIRED_LIBRARIES m)
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

configure_file("mdcm/Source/Common/mdcmConfigure.h.in" "${CMAKE_CURRENT_BINARY_DIR}/mdcm/Source/Common/mdcmConfigure.h")

#include_regular_expression("mdcm/Source/Common/mdcm.*$")

add_subdirectory(mdcm/Utilities)

set(MDCM_COMMON_SRCS
  mdcm/Source/Common/mdcmVersion.cxx
  mdcm/Source/Common/mdcmRegion.cxx
  mdcm/Source/Common/mdcmBoxRegion.cxx
  mdcm/Source/Common/mdcmEvent.cxx
  mdcm/Source/Common/mdcmMD5.cxx
  mdcm/Source/Common/mdcmBase64.cxx
  mdcm/Source/Common/mdcmSHA1.cxx
  mdcm/Source/Common/mdcmDummyValueGenerator.cxx
  mdcm/Source/Common/mdcmCryptoFactory.cxx
  mdcm/Source/Common/mdcmASN1.cxx
  mdcm/Source/Common/mdcmSubject.cxx
  mdcm/Source/Common/mdcmDirectory.cxx
  mdcm/Source/Common/mdcmFilename.cxx
  mdcm/Source/Common/mdcmFilenameGenerator.cxx
  mdcm/Source/Common/mdcmSwapCode.cxx
  mdcm/Source/Common/mdcmSystem.cxx
  mdcm/Source/Common/mdcmTrace.cxx
  mdcm/Source/Common/mdcmUnpacker12Bits.cxx)

if(WIN32)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    mdcm/Source/Common/mdcmCAPICryptographicMessageSyntax.cxx
    mdcm/Source/Common/mdcmCAPICryptoFactory.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    mdcm/Source/Common/mdcmOpenSSLP7CryptographicMessageSyntax.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL AND MDCM_HAVE_CMS_RECIPIENT_PASSWORD)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    mdcm/Source/Common/mdcmOpenSSLCryptoFactory.cxx
	mdcm/Source/Common/mdcmOpenSSLCryptographicMessageSyntax.cxx)
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(MDCM_DICT_SRCS
  mdcm/Source/DataDictionary/mdcmDicts.cxx
  mdcm/Source/DataDictionary/mdcmDictEntry.cxx
  mdcm/Source/DataDictionary/mdcmDefaultDicts.cxx
  mdcm/Source/DataDictionary/mdcmPrivateDefaultDicts.cxx
  mdcm/Source/DataDictionary/mdcmGlobal.cxx
  mdcm/Source/DataDictionary/mdcmGroupDict.cxx
  mdcm/Source/DataDictionary/mdcmDefaultGroupNames.cxx
  mdcm/Source/DataDictionary/mdcmUIDs.cxx
  mdcm/Source/DataDictionary/mdcmCSAHeaderDefaultDicts.cxx)


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
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmReader.strict.cxx # hook to call mdcmstrict API
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmTag.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmTagToVR.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmCodeString.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmFileSet.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmByteSwapFilter.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmUNExplicitImplicitDataElement.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmWriter.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmCSAHeader.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmMrProtocol.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmTransferSyntax.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmVM.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmPreamble.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmUNExplicitDataElement.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmCP246ExplicitDataElement.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmExplicitImplicitDataElement.cxx
  mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR16ExplicitDataElement.cxx
  ${MDCM_DSED3_SRCS})

set(MDCM_MSFF_SRCS
  mdcm/Source/MediaStorageAndFileFormat/mdcmFileStreamer.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageGenerator.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmDICOMDIRGenerator.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmFileExplicitFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmFileDerivation.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageFragmentSplitter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmTagPath.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSimpleSubjectWatcher.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPixmap.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmBitmap.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmRescaler.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageToImageFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmBitmapToBitmapFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapToPixmapFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangeTransferSyntax.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageApplyLookupTable.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmApplySupplementalLUT.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmFileDecompressLookupTable.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmOrientation.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmDataSetHelper.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangePlanarConfiguration.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmDirectionCosines.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSorter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmIPPSorter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSplitMosaicFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmUIDGenerator.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmUUIDGenerator.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmScanner.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmStrictScanner.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapWriter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageWriter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmStringFilter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageHelper.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmValidate.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImage.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageConverter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmImageCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG12Codec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmRLECodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG16Codec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG8Codec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPVRGCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmKAKADUCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPNMCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPGXCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmRAWCodec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmLookupTable.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmOverlay.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmCurve.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPhotometricInterpretation.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmPixelFormat.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentedPaletteColorLookupTable.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmStreamImageReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/#mdcmImageRegionReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmDirectoryHelper.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSegment.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSurface.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmMeshPrimitive.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentWriter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceWriter.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceReader.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSurfaceHelper.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSegmentHelper.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx
  mdcm/Source/MediaStorageAndFileFormat/mdcmSerieHelper.cxx
  mdcm/Utilities/mdcmrle/rle.cxx
  mdcm/Utilities/mdcmrle/info.cxx
  mdcm/Utilities/mdcmrle/io.cxx)

if(NOT MDCM_USE_SYSTEM_OPENJPEG)
  set_source_files_properties(mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx PROPERTIES COMPILE_FLAGS -DOPJ_STATIC)
endif()

if(NOT MDCM_USE_SYSTEM_CHARLS)
  set_source_files_properties(mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx PROPERTIES COMPILE_DEFINITIONS CHARLS_STATIC)
else()
  set_source_files_properties(mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx PROPERTIES COMPILE_FLAGS -DCHARLS_DLL)
endif()

#if(CMAKE_COMPILER_IS_GNUCXX AND MINGW)
#  set_target_properties(xxx PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc")
#endif()


















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

set(ALIZAMS_SRCS common/structures.cpp main.cpp)

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
  common/commonutils.cpp
  common/contourutils.cpp
  common/colorspace/colorspace.cpp
  common/filepath.cpp
  common/codecutils.cpp
  dicom/ultrasoundregionutils.cpp
  dicom/dicomutils.cpp
  dicom/prconfigutils.cpp
  dicom/splituihgridfilter.cpp
  dicom/spectroscopyutils.cpp
  dicom/srutils.cpp
  CG/camera.cpp
  CG/glwidget.cpp
  CG/testgl.cpp
  GUI/mainwindow.cpp
  GUI/labelwidget.cpp
  GUI/updateqtcommand.cpp
  GUI/iconutils.cpp
  GUI/histogramgen.cpp
  GUI/settingswidget.cpp
  GUI/qxtspanslider.cpp
  GUI/imagesbox.cpp
  GUI/toolbox.cpp
  GUI/animwidget.cpp
  GUI/toolbox2D.cpp
  GUI/sliderwidget.cpp
  GUI/zrangewidget.cpp
  GUI/lutwidget.cpp
  GUI/handleitem.cpp
  GUI/levelitem.cpp
  GUI/rectitem.cpp
  GUI/graphicsutils.cpp
  GUI/graphicspathitem.cpp
  GUI/graphicsview.cpp
  GUI/graphicswidget.cpp
  GUI/histogramview.cpp
  GUI/aboutwidget.cpp
  GUI/zoomwidget.cpp
  GUI/findrefdialog.cpp
  GUI/imageinfodialog.cpp
  GUI/srwidget.cpp
  browser/sqtree.cpp
  browser/browserwidget2.cpp
  GUI/aliza.cpp)

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} browser/ctkdialog.cpp)
endif()

if(NOT USE_QT_V_5 AND NOT USE_SYSTEM_GLEW_QT4)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} "${LOCAL_GLEW_PATH}/src/glew.c")
endif()

if(WIN32)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} alizams.rc)
endif()

set(ALIZAMS_QRCS alizams.qrc)

set(ALIZAMS_MOC_HRDS
  GUI/mainwindow.h
  browser/sqtree.h
  browser/browserwidget2.h
  GUI/findrefdialog.h
  GUI/imageinfodialog.h
  GUI/settingswidget.h
  GUI/qxtspanslider_p.h
  GUI/qxtspanslider.h
  GUI/imagesbox.h
  GUI/toolbox.h
  GUI/animwidget.h
  GUI/toolbox2D.h
  GUI/sliderwidget.h
  GUI/labelwidget.h
  GUI/zrangewidget.h
  GUI/lutwidget.h
  GUI/graphicsview.h
  GUI/graphicswidget.h
  GUI/histogramview.h
  GUI/zoomwidget.h
  GUI/aboutwidget.h
  GUI/srwidget.h
  GUI/aliza.h)

if(USE_QT_V_5)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} CG/glwidget-qt5.h)
else()
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} CG/glwidget-qt4.h)
endif()

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} browser/ctkdialog.h)
endif()

set(ALIZAMS_UIS
  GUI/settingswidget.ui
  GUI/mainwindow.ui
  GUI/aboutwidget.ui
  browser/sqtree.ui
  browser/browserwidget2.ui
  GUI/findrefdialog.ui
  GUI/imagesbox.ui
  GUI/toolbox.ui
  GUI/animwidget.ui
  GUI/toolbox2D.ui
  GUI/sliderwidget.ui
  GUI/srwidget.ui
  GUI/imageinfodialog.ui)

if(NOT USE_MEDIASTORAGE_MODE)
  set(ALIZAMS_UIS ${ALIZAMS_UIS} browser/ctkdialog.ui)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/common"
  "${CMAKE_CURRENT_SOURCE_DIR}/GUI"
  "${CMAKE_CURRENT_SOURCE_DIR}/dicom"
  ${CMAKE_CURRENT_BINARY_DIR})

# Available defs: -DDISABLE_SIMDMATH to disable SSE2 math
if(WIN32)
  if(MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400)
    add_definitions(
     -D_CRT_SECURE_NO_DEPRECATE
     -D_CRT_NONSTDC_NO_DEPRECATE
     -D_CRT_SECURE_NO_WARNINGS
     -D_SCL_SECURE_NO_DEPRECATE
     -D_SCL_SECURE_NO_WARNINGS)
  endif()
else()
  if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    link_directories(/usr/local/lib)
  endif()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    add_definitions(-DDISABLE_SIMDMATH)
  endif()
endif()






set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${MDCM_COMMON_SRCS} ${MDCM_DICT_SRCS} ${MDCM_DSED_SRCS} ${MDCM_MSFF_SRCS})

#set(ALIZAMS_LINK_LIBRARIES ${MDCM_LIBRARIES} ${ITK_LIBRARIES})
set(ALIZAMS_LINK_LIBRARIES ${ITK_LIBRARIES})

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
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES})
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
  link_directories("bin")
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES} mdcmjpeg8 mdcmjpeg12 mdcmjpeg16 mdcmopenjp2 mdcmzlib mdcmuuid mdcmcharls)
endif()

install(TARGETS alizams RUNTIME DESTINATION "bin")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/icons" DESTINATION "share")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/applications" DESTINATION "share")

