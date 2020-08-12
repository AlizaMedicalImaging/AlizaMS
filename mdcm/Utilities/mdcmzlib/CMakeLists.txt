if(NOT ZLIB_NAMESPACE)
  set(ZLIB_NAMESPACE "ZLIB")
  set(ZLIB_STANDALONE 1)
endif()

string(TOLOWER ${ZLIB_NAMESPACE} ZLIB_LIBRARY_NAME)

project(${ZLIB_NAMESPACE} C)

include_regular_expression("^.*$")

set(ZLIB_SRCS
  adler32.c
  compress.c
  crc32.c
  deflate.c
  gzio.c
  infback.c
  inffast.c
  inflate.c
  inftrees.c
  trees.c
  uncompr.c
  zutil.c)

# for windows add the .def and .rc files to the source list if building shared libs
if(WIN32)
  if(BUILD_SHARED_LIBS)
    set(ZLIB_DLL 1)
    if(NOT UNIX)
      if(NOT BORLAND)
        if(NOT MINGW)
          set(ZLIB_SRCS ${ZLIB_SRCS} zlib.def zlib.rc)
        endif()
      endif() 
    endif()   
  endif()
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/zconf.in.h
  ${CMAKE_CURRENT_BINARY_DIR}/zconf.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(${ZLIB_LIBRARY_NAME} ${ZLIB_SRCS})
#set_target_properties(${ZLIB_LIBRARY_NAME} PROPERTIES ${MDCM_LIBRARY_PROPERTIES})
set_target_properties(${ZLIB_LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE C)


