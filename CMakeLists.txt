##########################################################################
#
# Aliza MS
#
##########################################################################

cmake_minimum_required(VERSION 3.13)
#
# Note, for CMAKE_CXX_STANDARD:
# For C++20 version 3.12 is required.
# For C++23 version 3.20 is required.
# For C++26 version 3.25 is required.
# A compiler should support at least C++14.
#

##########################################################################
#
#
#
#

# https://cmake.org/cmake/help/v3.0/policy/CMP0003.html
# Libraries linked via full path no longer produce linker search paths
cmake_policy(SET CMP0003 NEW)

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

# https://cmake.org/cmake/help/v3.19/policy/CMP0072.html
# FindOpenGL prefers GLVND by default when available.
# https://cmake.org/cmake/help/v3.19/module/FindOpenGL.html
if (POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()
#
#
#
#
##########################################################################

project(ALIZAMS)

##########################################################################
#
#
# Options
#
#
#
if(APPLE)
  set(ALIZAMS_OSX_BUNDLE_VERSION "1.10.0")
endif()

# Qt major version, required.
set(ALIZA_QT_VERSION "QT_VERSION_NOT_SELECTED" CACHE STRING "Qt major version")
set_property(CACHE ALIZA_QT_VERSION PROPERTY STRINGS "QT_VERSION_NOT_SELECTED" "4" "5" "6")

# There is no guarantee that the option will work,
# check "__cplusplus" (s. main.cpp PRINT_HOST_INFO define).
set(ALIZA_CXX_STANDARD "Not set" CACHE STRING "Select C++ standard, usually not requied")
set_property(CACHE ALIZA_CXX_STANDARD PROPERTY STRINGS "Not set" "14" "17" "20" "23" "26")

# Force-disable SSE/SSE2.
option(ALIZA_DISABLE_SIMDMATH "Disable SSE/SSE2" OFF)

if(NOT APPLE)
  # Use OpenGL Core profile.
  option(ALIZA_COREGL "Use OpenGL Core Profile" ON)
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

option(ALIZA_VERBOSE "Print messages to std::cout" OFF)

# Temporary
option(TMP_USE_53_SPATIAL_ENUMS "For internal use only, do not set to FALSE" TRUE)
mark_as_advanced(TMP_USE_53_SPATIAL_ENUMS)

#
#
# MDCM
#
#

if(WIN32)
  option(MDCM_USE_WIN32_UNC "Unicode, long file path (Windows)" ON)
endif()

option(MDCM_NO_MESSAGES "Disable all notifications" OFF)

option(MDCM_USE_SYSTEM_ZLIB "Use system zlib" OFF)

if(UNIX AND NOT APPLE)
  option(MDCM_USE_SYSTEM_UUID "Use system uuid" OFF)
endif()

option(MDCM_USE_SYSTEM_OPENJPEG "Use system OpenJPEG" OFF)

option(MDCM_USE_SYSTEM_CHARLS "Use system CharLS" OFF)

if(FALSE)
#
# OpenSSL is currently not used, don't enable!
#
  option(MDCM_USE_SYSTEM_OPENSSL "Use system OpenSSL" OFF)
  mark_as_advanced(MDCM_USE_SYSTEM_OPENSSL)
endif()
#
#
#
#
#
#
##########################################################################

# Required for x86 32 bit GCC build only.
if(NOT WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    if(NOT ALIZA_DISABLE_SIMDMATH)
      if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2 ")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 ")
      endif()
    endif()
  endif()
endif()

##########################################################################
#
#
# C/C++ flags (this section is for informational purposes only)
#
#
#
# Note: For any build configuration other than 'Debug' (including
# non-standard ones), the NDEBUG is defined automatically; there is
# no need to specify it via C/C++ flags (but there is no harm in
# this, and no need to change the standard configurations).
#
#
#
get_cmake_property(TMP_IS_MULTI_CONFIG_GENERATOR GENERATOR_IS_MULTI_CONFIG)
if(TMP_IS_MULTI_CONFIG_GENERATOR)
  foreach(TMP_MCCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    if(NOT ${TMP_MCCONFIG} STREQUAL "")
      string(TOUPPER ${TMP_MCCONFIG} TMP_MCCONGIG_UPPER)
      if((NOT ${TMP_MCCONGIG_UPPER} STREQUAL "DEBUG") AND
         (NOT ${TMP_MCCONGIG_UPPER} STREQUAL "RELEASE") AND
         (NOT ${TMP_MCCONGIG_UPPER} STREQUAL "RELWITHDEBINFO") AND
         (NOT ${TMP_MCCONGIG_UPPER} STREQUAL "MINSIZEREL"))
        message(STATUS "Non-standard ${TMP_MCCONFIG} configuration")
      endif()
      message(STATUS "Configuration: ${TMP_MCCONFIG}")
      message(STATUS "  CXX flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${TMP_MCCONGIG_UPPER}}")
      message(STATUS "  C flags:   ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${TMP_MCCONGIG_UPPER}}")
    else()
      message(WARNING "Empty configuration in CMAKE_CONFIGURATION_TYPES")
    endif()
  endforeach()
else()
  if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} TMP_BUILD_TYPE)
    if((NOT ${TMP_BUILD_TYPE} STREQUAL "DEBUG") AND
       (NOT ${TMP_BUILD_TYPE} STREQUAL "RELEASE") AND
       (NOT ${TMP_BUILD_TYPE} STREQUAL "RELWITHDEBINFO") AND
       (NOT ${TMP_BUILD_TYPE} STREQUAL "MINSIZEREL"))
      message(STATUS "Non-standard ${CMAKE_BUILD_TYPE} configuration")
    endif()
    set(TMP_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${TMP_BUILD_TYPE}}")
    set(TMP_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${TMP_BUILD_TYPE}}")
    message(STATUS "Configuration ${CMAKE_BUILD_TYPE}")
    message(STATUS "  CXX flags: ${TMP_CXX_FLAGS}")
    message(STATUS "  C flags:   ${TMP_C_FLAGS}")
  else()
    message(WARNING "CMAKE_BUILD_TYPE is not set")
    message(STATUS "  CXX flags: ${CMAKE_CXX_FLAGS}")
    message(STATUS "  C flags:   ${CMAKE_C_FLAGS}")
  endif()
endif()
#
#
#
##########################################################################

message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "System name:      ${CMAKE_SYSTEM_NAME}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE PATH "Output directory for executables")
  mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
endif()
if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE PATH "Output directory for libraries")
  mark_as_advanced(LIBRARY_OUTPUT_PATH)
endif()

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
  if(ALIZA_CXX_STANDARD STREQUAL "14")
    set(CMAKE_CXX_STANDARD 14)
    message(STATUS "Tried to set C++14")
  elseif(ALIZA_CXX_STANDARD STREQUAL "17")
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "Tried to set C++17")
  elseif(ALIZA_CXX_STANDARD STREQUAL "20")
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "Tried to set C++20")
  elseif(ALIZA_CXX_STANDARD STREQUAL "23")
    set(CMAKE_CXX_STANDARD 23)
    message(STATUS "Tried to set C++23")
  elseif(ALIZA_CXX_STANDARD STREQUAL "26")
    set(CMAKE_CXX_STANDARD 26)
    message(STATUS "Tried to set C++26")
  endif()
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

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

if(MDCM_USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
  include_directories(${ZLIB_INCLUDE_DIR})
  set(MDCM_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
else()
  set(MDCM_ZLIB_LIBRARIES mdcmzlib)
endif()

if(UNIX)
  if(MDCM_USE_SYSTEM_UUID)
    find_package(UUID REQUIRED)
    set(MDCM_UUID_LIBRARIES ${UUID_LIBRARIES})
  else()
    set(MDCM_UUID_LIBRARIES mdcmuuid)
  endif()
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  find_package(OpenSSL REQUIRED)
  message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
  message(STATUS "OpenSSL include:   ${OPENSSL_INCLUDE_DIR}")
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

include("${CMAKE_ROOT}/Modules/TestBigEndian.cmake")
TEST_BIG_ENDIAN(MDCM_WORDS_BIGENDIAN)

include("${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake")
include("${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake")

#FIXME
macro(CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES("${UUID_INCLUDES};${FILE}" ${VARIABLE})
  if(${VARIABLE})
    set(UUID_INCLUDES ${UUID_INCLUDES} ${FILE})
  endif()
endmacro()
#

set(MDCM_MAJOR_VERSION 1)
set(MDCM_MINOR_VERSION 6)
set(MDCM_BUILD_VERSION 7)
set(MDCM_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}.${MDCM_BUILD_VERSION}")
set(MDCM_API_VERSION "${MDCM_MAJOR_VERSION}.${MDCM_MINOR_VERSION}")

check_include_file_concat("byteswap.h" MDCM_HAVE_BYTESWAP_FILE_H)
# s. GDCM bug #324
if(MDCM_HAVE_BYTESWAP_FILE_H)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("#include <byteswap.h>\nint main(){return bswap_32(42);}" MDCM_HAVE_BYTESWAP_H)
endif()

check_include_file("rpc.h" MDCM_HAVE_RPC_H)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("int main(){const char *f =__FUNCTION__;return 0;}" MDCM_CXX_HAS_FUNCTION)

if(MDCM_USE_SYSTEM_OPENSSL)
  set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS})
  check_cxx_source_compiles("#include <openssl/cms.h>\nint main(){CMS_add0_recipient_password(0,0,0,0,0,0,0);return 0;}" MDCM_HAVE_CMS_RECIPIENT_PASSWORD)
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

if(MDCM_USE_SYSTEM_OPENSSL)
  include_directories(${OPENSSL_INCLUDE_DIR})
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
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/mdcm/Utilities")
#
#
#
#
#

set(MDCM_COMMON_SRCS
  "mdcm/Source/Common/mdcmVersion.cxx"
  "mdcm/Source/Common/mdcmEvent.cxx"
  "mdcm/Source/Common/mdcmBase64.cxx"
  "mdcm/Source/Common/mdcmSHA1.cxx"
  "mdcm/Source/Common/mdcmCryptoFactory.cxx"
  "mdcm/Source/Common/mdcmASN1.cxx"
  "mdcm/Source/Common/mdcmSubject.cxx"
  "mdcm/Source/Common/mdcmFilename.cxx"
  "mdcm/Source/Common/mdcmFilenameGenerator.cxx"
  "mdcm/Source/Common/mdcmSwapCode.cxx"
  "mdcm/Source/Common/mdcmSystem.cxx"
  "mdcm/Source/Common/mdcmUnpacker12Bits.cxx")

if(WIN32)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    "mdcm/Source/Common/mdcmCAPICryptographicMessageSyntax.cxx"
    "mdcm/Source/Common/mdcmCAPICryptoFactory.cxx")
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    "mdcm/Source/Common/mdcmOpenSSLP7CryptographicMessageSyntax.cxx")
endif()

if(MDCM_USE_SYSTEM_OPENSSL AND MDCM_HAVE_CMS_RECIPIENT_PASSWORD)
  set(MDCM_COMMON_SRCS ${MDCM_COMMON_SRCS}
    "mdcm/Source/Common/mdcmOpenSSLCryptoFactory.cxx"
    "mdcm/Source/Common/mdcmOpenSSLCryptographicMessageSyntax.cxx")
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(MDCM_DICT_SRCS
  "mdcm/Source/DataDictionary/mdcmDicts.cxx"
  "mdcm/Source/DataDictionary/mdcmDefaultDicts.cxx"
  "mdcm/Source/DataDictionary/mdcmPrivateDefaultDicts.cxx"
  "mdcm/Source/DataDictionary/mdcmGlobal.cxx"
  "mdcm/Source/DataDictionary/mdcmUIDs.cxx"
  "mdcm/Source/DataDictionary/mdcmCSAHeaderDefaultDicts.cxx")

set(MDCM_DSED_SRCS
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmByteValue.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmDataSet.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmExplicitDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmFile.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmFileMetaInformation.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmFragment.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmImplicitDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmMediaStorage.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmPrivateTag.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmReader.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmSequenceOfFragments.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmSequenceOfItems.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmValue.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmTag.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmCodeString.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmByteSwapFilter.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmWriter.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmCSAHeader.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmMrProtocol.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmTransferSyntax.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmVM.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmPreamble.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmUNExplicitDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmCP246ExplicitDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmExplicitImplicitDataElement.cxx"
  "mdcm/Source/DataStructureAndEncodingDefinition/mdcmVR16ExplicitDataElement.cxx")

# Code for segments and surfaces is currently unused.
set(MDCM_MSFF_SRCS
  "mdcm/Source/MediaStorageAndFileFormat/mdcmIconImageFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmFileExplicitFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageFragmentSplitter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmSimpleSubjectWatcher.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPixmap.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmBitmap.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmRescaler.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageToImageFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmBitmapToBitmapFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapToPixmapFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangeTransferSyntax.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageApplyLookupTable.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmApplySupplementalLUT.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmFileDecompressLookupTable.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmOrientation.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmDataSetHelper.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageChangePlanarConfiguration.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmDirectionCosines.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmSplitMosaicFilter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmUIDGenerator.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmUUIDGenerator.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmScanner.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapReader.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageReader.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPixmapWriter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageWriter.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageHelper.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImage.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmImageCodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG12Codec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmRLE.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmRLECodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG16Codec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG8Codec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGCodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmRAWCodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmEncapsulatedRAWCodec.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmLookupTable.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmOverlay.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmCurve.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPhotometricInterpretation.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmPixelFormat.cxx"
  "mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx")

if(NOT MDCM_USE_SYSTEM_OPENJPEG)
  set_source_files_properties("mdcm/Source/MediaStorageAndFileFormat/mdcmJPEG2000Codec.cxx" PROPERTIES COMPILE_DEFINITIONS OPJ_STATIC)
endif()

if(NOT MDCM_USE_SYSTEM_CHARLS)
  set_source_files_properties("mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx" PROPERTIES COMPILE_DEFINITIONS CHARLS_STATIC)
else()
  set_source_files_properties("mdcm/Source/MediaStorageAndFileFormat/mdcmJPEGLSCodec.cxx" PROPERTIES COMPILE_DEFINITIONS CHARLS_DLL)
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

#
#
#
find_package(OpenGL REQUIRED)
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
  endif()
  set(USE_QT_V_4 TRUE)
else()
  message(FATAL_ERROR "Can not determine Qt version to use")
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

set(ALIZAMS_SRCS
  "common/mmath.cpp"
  "common/structures.cpp"
  "main.cpp")

if(NOT ALIZA_USE_SYSTEM_BULLET)
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/b")
  set(BULLET_SRCS
    "b/LinearMath/btAlignedAllocator.cpp"
    "b/LinearMath/btConvexHull.cpp"
    "b/LinearMath/btConvexHullComputer.cpp"
    "b/LinearMath/btGeometryUtil.cpp"
    "b/LinearMath/btPolarDecomposition.cpp"
    "b/LinearMath/btQuickprof.cpp"
    "b/LinearMath/btReducedVector.cpp"
    "b/LinearMath/btSerializer.cpp"
    "b/LinearMath/btSerializer64.cpp"
    "b/LinearMath/btThreads.cpp"
    "b/LinearMath/btVector3.cpp"
    "b/LinearMath/TaskScheduler/btTaskScheduler.cpp"
    "b/LinearMath/TaskScheduler/btThreadSupportPosix.cpp"
    "b/LinearMath/TaskScheduler/btThreadSupportWin32.cpp"
    "b/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp"
    "b/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp"
    "b/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp"
    "b/BulletCollision/BroadphaseCollision/btDbvt.cpp"
    "b/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp"
    "b/BulletCollision/BroadphaseCollision/btDispatcher.cpp"
    "b/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp"
    "b/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp"
    "b/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp"
    "b/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp"
    "b/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp"
    "b/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.cpp"
    "b/BulletCollision/CollisionDispatch/btCollisionObject.cpp"
    "b/BulletCollision/CollisionDispatch/btCollisionWorld.cpp"
    "b/BulletCollision/CollisionDispatch/btCollisionWorldImporter.cpp"
    "b/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp"
    "b/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btGhostObject.cpp"
    "b/BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp"
    "b/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp"
    "b/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
    "b/BulletCollision/CollisionDispatch/btManifoldResult.cpp"
    "b/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp"
    "b/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp"
    "b/BulletCollision/CollisionDispatch/btUnionFind.cpp"
    "b/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp"
    "b/BulletCollision/CollisionShapes/btBoxShape.cpp"
    "b/BulletCollision/CollisionShapes/btBox2dShape.cpp"
    "b/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp"
    "b/BulletCollision/CollisionShapes/btCapsuleShape.cpp"
    "b/BulletCollision/CollisionShapes/btCollisionShape.cpp"
    "b/BulletCollision/CollisionShapes/btCompoundShape.cpp"
    "b/BulletCollision/CollisionShapes/btConcaveShape.cpp"
    "b/BulletCollision/CollisionShapes/btConeShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvexHullShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvexInternalShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp"
    "b/BulletCollision/CollisionShapes/btConvexShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvex2dShape.cpp"
    "b/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp"
    "b/BulletCollision/CollisionShapes/btCylinderShape.cpp"
    "b/BulletCollision/CollisionShapes/btEmptyShape.cpp"
    "b/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp"
    "b/BulletCollision/CollisionShapes/btMiniSDF.cpp"
    "b/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp"
    "b/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp"
    "b/BulletCollision/CollisionShapes/btMultiSphereShape.cpp"
    "b/BulletCollision/CollisionShapes/btOptimizedBvh.cpp"
    "b/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp"
    "b/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp"
    "b/BulletCollision/CollisionShapes/btSdfCollisionShape.cpp"
    "b/BulletCollision/CollisionShapes/btShapeHull.cpp"
    "b/BulletCollision/CollisionShapes/btSphereShape.cpp"
    "b/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp"
    "b/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp"
    "b/BulletCollision/CollisionShapes/btTetrahedronShape.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleBuffer.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleCallback.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleMesh.cpp"
    "b/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp"
    "b/BulletCollision/CollisionShapes/btUniformScalingShape.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp"
    "b/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp")
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} ${BULLET_SRCS})
else()
  find_package(Bullet REQUIRED COMPONENTS LinearMath BulletCollision)
  message(STATUS "Bullet include dir: ${BULLET_INCLUDE_DIR}")
  message(STATUS "Bullet libraries:   ${BULLET_LIBRARIES}")
  message(STATUS "Bullet definitions: ${BULLET_DEFINITIONS}")
  add_definitions(${BULLET_DEFINITIONS})
  include_directories(${BULLET_INCLUDE_DIR})
endif()

#
#
#
#
#
if(ALIZA_USE_SYSTEM_LCMS2)
  find_package(LCMS2 REQUIRED)
  message(STATUS "LCMS2: ${LCMS2_LIBRARIES}")
else()
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/alizalcms")
  message(STATUS "LCMS2: alizalcms2")
endif()
#
#
#
#
#

set(ALIZAMS_SRCS ${ALIZAMS_SRCS}
  "common/commonutils.cpp"
  "common/contourutils.cpp"
  "common/colorspace/colorspace.cpp"
  "common/codecutils.cpp"
  "dicom/ultrasoundregionutils.cpp"
  "dicom/loaddicom_t.cpp"
  "dicom/loaddicom.cpp"
  "dicom/dicomutils.cpp"
  "dicom/prconfigutils.cpp"
  "dicom/splituihgridfilter.cpp"
  "dicom/spectroscopyutils.cpp"
  "dicom/srutils.cpp"
  "CG/camera.cpp"
  "CG/glwidget.cpp"
  "CG/testgl.cpp"
  "GUI/mainwindow.cpp"
  "GUI/updateqtcommand.cpp"
  "GUI/iconutils.cpp"
  "GUI/histogramgen.cpp"
  "GUI/settingswidget.cpp"
  "GUI/qxtspanslider.cpp"
  "GUI/imagesbox.cpp"
  "GUI/toolbox.cpp"
  "GUI/animwidget.cpp"
  "GUI/toolbox2D.cpp"
  "GUI/sliderwidget.cpp"
  "GUI/zrangewidget.cpp"
  "GUI/lutwidget.cpp"
  "GUI/handleitem.cpp"
  "GUI/levelitem.cpp"
  "GUI/rectitem.cpp"
  "GUI/graphicsutils.cpp"
  "GUI/graphicspathitem.cpp"
  "GUI/graphicsview.cpp"
  "GUI/studygraphicswidget.cpp"
  "GUI/studygraphicsview.cpp"
  "GUI/graphicswidget.cpp"
  "GUI/histogramview.cpp"
  "GUI/aboutwidget.cpp"
  "GUI/zoomwidget.cpp"
  "GUI/findrefdialog.cpp"
  "GUI/infodialog.cpp"
  "GUI/imageinfodialog.cpp"
  "GUI/srwidget.cpp"
  "GUI/matrixbutton.cpp"
  "GUI/studyframewidget.cpp"
  "GUI/studyviewwidget.cpp"
  "browser/sqtree.cpp"
  "browser/browserwidget2.cpp"
  "browser/helpwidget.cpp"
  "browser/anonymazerwidget2.cpp"
  "GUI/aliza.cpp")

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} "browser/ctkdialog.cpp")
endif()

if(USE_QT_V_4 AND NOT ALIZA_QT4_SYSTEM_GLEW)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} "${LOCAL_GLEW_PATH}/src/glew.c")
endif()

if(WIN32)
  set(ALIZAMS_SRCS ${ALIZAMS_SRCS} "alizams.rc")
endif()

set(ALIZAMS_QRCS "alizams.qrc")

set(ALIZAMS_MOC_HRDS
  "GUI/mainwindow.h"
  "browser/sqtree.h"
  "browser/browserwidget2.h"
  "browser/helpwidget.h"
  "browser/anonymazerwidget2.h"
  "dicom/loaddicom_t.h"
  "GUI/findrefdialog.h"
  "GUI/infodialog.h"
  "GUI/imageinfodialog.h"
  "GUI/settingswidget.h"
  "GUI/qxtspanslider_p.h"
  "GUI/qxtspanslider.h"
  "GUI/imagesbox.h"
  "GUI/toolbox.h"
  "GUI/animwidget.h"
  "GUI/toolbox2D.h"
  "GUI/sliderwidget.h"
  "GUI/zrangewidget.h"
  "GUI/lutwidget.h"
  "GUI/matrixbutton.h"
  "GUI/studyframewidget.h"
  "GUI/studyviewwidget.h"
  "GUI/graphicsview.h"
  "GUI/graphicswidget.h"
  "GUI/studygraphicsview.h"
  "GUI/studygraphicswidget.h"
  "GUI/histogramview.h"
  "GUI/zoomwidget.h"
  "GUI/aboutwidget.h"
  "GUI/srwidget.h"
  "GUI/aliza.h")

if(APPLE)
  if(USE_QT_V_6)
    set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt5-core.h")
  else()
    message(FATAL_ERROR "Only Qt6 is supported with macOS")
  endif()
else()
  if(USE_QT_V_6)
    if(ALIZA_COREGL)
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt5-core.h")
    else()
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt5.h")
    endif()
  elseif(USE_QT_V_5)
    if(ALIZA_COREGL)
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt5-core.h")
    else()
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt5.h")
    endif()
  elseif(USE_QT_V_4)
    if(ALIZA_COREGL)
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt4-core.h")
    else()
      set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "CG/glwidget-qt4.h")
    endif()
  else()
    message(FATAL_ERROR "Can not determine Qt version to use")
  endif()
endif()

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_MOC_HRDS ${ALIZAMS_MOC_HRDS} "browser/ctkdialog.h")
endif()

set(ALIZAMS_UIS
  "GUI/settingswidget.ui"
  "GUI/mainwindow.ui"
  "GUI/aboutwidget.ui"
  "browser/sqtree.ui"
  "browser/browserwidget2.ui"
  "browser/helpwidget.ui"
  "browser/anonymazerwidget2.ui"
  "GUI/findrefdialog.ui"
  "GUI/infodialog.ui"
  "GUI/imagesbox.ui"
  "GUI/toolbox.ui"
  "GUI/animwidget.ui"
  "GUI/toolbox2D.ui"
  "GUI/sliderwidget.ui"
  "GUI/srwidget.ui"
  "GUI/studyframewidget.ui"
  "GUI/studyviewwidget.ui"
  "GUI/imageinfodialog.ui")

if(NOT ALIZA_MEDIASTORAGE_MODE)
  set(ALIZAMS_UIS ${ALIZAMS_UIS} "browser/ctkdialog.ui")
endif()

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/common"
  "${CMAKE_CURRENT_SOURCE_DIR}/GUI"
  "${CMAKE_CURRENT_SOURCE_DIR}/dicom"
  "${CMAKE_CURRENT_BINARY_DIR}")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  link_directories("/usr/local/lib")
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
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${BULLET_LIBRARIES})
endif()

if(ALIZA_USE_SYSTEM_LCMS2)
  include_directories(${LCMS2_INCLUDE_DIRS})
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${LCMS2_LIBRARIES})
else()
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} alizalcms2)
endif()

if(MDCM_USE_SYSTEM_OPENSSL)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} ${OPENSSL_LIBRARIES})
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
    ${ALIZAMS_QRCS_RCC}
    Win32AppUtf8.manifest)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES} crypt32 rpcrt4)
elseif(APPLE)
  set(ALIZAMS_LINK_LIBRARIES ${ALIZAMS_LINK_LIBRARIES})
  set(ALIZAMS_OSX_ICON_NAME "AlizaMS.icns")
  set(ALIZAMS_OSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/${ALIZAMS_OSX_ICON_NAME}")
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
    MACOSX_BUNDLE_COPYRIGHT "© 2015-2026 Aliza Medical Imaging")
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
  install(TARGETS alizams RUNTIME DESTINATION "bin")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/icons" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/applications" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/metainfo" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/man" DESTINATION "share")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/alizams" DESTINATION "share")
endif()

# Safely add NDEBUG
target_compile_definitions(alizams PRIVATE $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

if(TMP_USE_53_SPATIAL_ENUMS)
  target_compile_definitions(alizams PRIVATE TMP_USE_53_SPATIAL_ENUMS)
endif()

if(UNIX AND NOT APPLE)
  if(ALIZA_QT5_6_PLATFORM_XCB)
    target_compile_definitions(alizams PRIVATE FORCE_PLATFORM_XCB)
  endif()
endif()

if(ALIZA_DISABLE_SIMDMATH)
  target_compile_definitions(alizams PRIVATE DISABLE_SIMDMATH)
else()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    target_compile_definitions(alizams PRIVATE DISABLE_SIMDMATH)
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    target_compile_definitions(alizams PRIVATE DISABLE_SIMDMATH)
  endif()
endif()

if(ALIZA_VERBOSE)
  target_compile_definitions(alizams PRIVATE ALIZA_VERBOSE)
endif()

if(WIN32)
  if(MDCM_USE_WIN32_UNC)
    target_compile_definitions(alizams PRIVATE MDCM_WIN32_UNC)
  endif()
endif()

if(MDCM_NO_MESSAGES)
  target_compile_definitions(alizams PRIVATE MDCM_NO_MESSAGES)
endif()

if(MDCM_USE_SYSTEM_CHARLS)
  if(CHARLS_LOW)
    target_compile_definitions(alizams PRIVATE MDCM_CHARLS_LOW)
  endif()
endif()

# tmp
target_compile_definitions(alizams PRIVATE ALIZA_LOAD_DCM_THREAD)

if(APPLE)
  target_compile_definitions(alizams PRIVATE ALIZA_GL_3_2_CORE)
  target_compile_definitions(alizams PUBLIC GL_SILENCE_DEPRECATION)
else()
  if(ALIZA_COREGL)
    target_compile_definitions(alizams PRIVATE ALIZA_GL_3_2_CORE)
  endif()
endif()

if(USE_QT_V_4)
  if(NOT ALIZA_QT4_SYSTEM_GLEW)
    target_compile_definitions(alizams PUBLIC GLEW_STATIC)
  endif()
endif()

if(NOT ALIZA_MEDIASTORAGE_MODE)
  target_compile_definitions(alizams PRIVATE USE_WORKSTATION_MODE)
endif()

if(ALIZA_USE_SYSTEM_LCMS2)
  target_compile_definitions(alizams PRIVATE ALIZA_USE_SYSTEM_LCMS2)
endif()

target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES})

