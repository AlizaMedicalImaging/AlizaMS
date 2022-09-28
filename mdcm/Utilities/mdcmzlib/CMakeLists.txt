if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
  cmake_minimum_required(VERSION 3.0.0)
endif()

if(NOT ZLIB_NAMESPACE)
  set(ZLIB_NAMESPACE "ZLIB")
  set(ZLIB_STANDALONE 1)
endif()

string(TOLOWER ${ZLIB_NAMESPACE} ZLIB_LIBRARY_NAME)

project(${ZLIB_NAMESPACE} C)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    if (FALSE) # XXX debug suffixes.
    set(CMAKE_DEBUG_POSTFIX "d")
    endif ()
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# mangle
set(Z_PREFIX TRUE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

if(WIN32 AND BUILD_SHARED_LIBS AND NOT MINGW)
  set(ZLIB_DLL 1)
  list(APPEND ZLIB_SRCS win32/zlib1.rc)
endif()

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
add_library(${ZLIB_LIBRARY_NAME} ${ZLIB_SRCS})
set_target_properties(${ZLIB_LIBRARY_NAME} PROPERTIES ${MDCM_LIBRARY_PROPERTIES})
set_target_properties(${ZLIB_LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE C)

if(NOT ZLIB_INSTALL_NO_LIBRARIES)
  install(TARGETS ${ZLIB_LIBRARY_NAME}
    EXPORT ${MDCM_TARGETS_NAME}
    RUNTIME DESTINATION ${ZLIB_INSTALL_BIN_DIR} COMPONENT Applications
    LIBRARY DESTINATION ${ZLIB_INSTALL_LIB_DIR} COMPONENT Libraries
    ARCHIVE DESTINATION ${ZLIB_INSTALL_LIB_DIR} COMPONENT DebugDevel
    ${CPACK_NAMELINK_TYPE})
endif()


