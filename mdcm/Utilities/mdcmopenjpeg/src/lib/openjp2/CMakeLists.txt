include_regular_expression("^.*$")

set(OPENJPEG_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/thread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bio.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cio.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dwt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dwt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event.c
  ${CMAKE_CURRENT_SOURCE_DIR}/event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ht_dec.c
  ${CMAKE_CURRENT_SOURCE_DIR}/image.c
  ${CMAKE_CURRENT_SOURCE_DIR}/image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/invert.c
  ${CMAKE_CURRENT_SOURCE_DIR}/invert.h
  ${CMAKE_CURRENT_SOURCE_DIR}/j2k.c
  ${CMAKE_CURRENT_SOURCE_DIR}/j2k.h
  ${CMAKE_CURRENT_SOURCE_DIR}/jp2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/jp2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mct.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mct.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mqc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mqc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mqc_inl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_clock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_clock.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/t1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/t1.h
  ${CMAKE_CURRENT_SOURCE_DIR}/t2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/t2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tcd.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tcd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tgt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tgt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/function_list.c
  ${CMAKE_CURRENT_SOURCE_DIR}/function_list.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_codec.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_includes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_intmath.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_malloc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/opj_malloc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sparse_array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sparse_array.h
)

option(OPJ_DISABLE_TPSOT_FIX "Disable TPsot==TNsot fix. See https://github.com/uclouvain/openjpeg/issues/254." OFF)
mark_as_advanced(OPJ_DISABLE_TPSOT_FIX)
if(OPJ_DISABLE_TPSOT_FIX)
  add_definitions(-DOPJ_DISABLE_TPSOT_FIX)
endif()

# Special case for old i586-mingw32msvc-gcc cross compiler
if(NOT WIN32 AND CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER MATCHES ".*mingw32msvc.*")
  set(WIN32 YES)
endif()

message(STATUS "OpenJPEG current bin. dir: ${CMAKE_CURRENT_BINARY_DIR}")

add_library(${OPENJPEG_LIBRARY_NAME} ${OPENJPEG_SRCS})
target_include_directories(${OPENJPEG_LIBRARY_NAME} BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${OPENJPEG_LIBRARY_NAME} BEFORE PRIVATE "${${OPENJPEG_NAMESPACE}_BINARY_DIR}/src/lib/openjp2")

# Safely add NDEBUG
target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
# Only static libs here
target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE OPJ_STATIC)

# This removes MSVC warnings
if(WIN32)
  if(NOT BORLAND)
    if(NOT CYGWIN)
      if(NOT MINGW)
        target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE
          _CRT_FAR_MAPPINGS_NO_DEPRECATE
          _CRT_IS_WCTYPE_NO_DEPRECATE
          _CRT_MANAGED_FP_NO_DEPRECATE
          _CRT_NONSTDC_NO_DEPRECATE
          _CRT_SECURE_NO_DEPRECATE
          _CRT_SECURE_NO_DEPRECATE_GLOBALS
          _CRT_SETERRORMODE_BEEP_SLEEP_NO_DEPRECATE
          _CRT_TIME_FUNCTIONS_NO_DEPRECATE
          _CRT_VCCLRIT_NO_DEPRECATE
          _SCL_SECURE_NO_DEPRECATE)
      endif()
    endif()
  endif()
endif()


if(UNIX)
  target_link_libraries(${OPENJPEG_LIBRARY_NAME} m)
endif()
set_target_properties(${OPENJPEG_LIBRARY_NAME} PROPERTIES ${OPENJPEG_LIBRARY_PROPERTIES})

#################################################################################
# threading configuration
#################################################################################
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

option(OPJ_USE_THREAD "Build with thread/mutex support " ON)
mark_as_advanced(OPJ_USE_THREAD)
if(NOT OPJ_USE_THREAD)
  target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE MUTEX_stub)
endif()

find_package(Threads QUIET)

if(OPJ_USE_THREAD AND WIN32 AND NOT Threads_FOUND)
  target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE MUTEX_win32)
  set(Threads_FOUND YES)
endif()

if(OPJ_USE_THREAD AND Threads_FOUND AND CMAKE_USE_WIN32_THREADS_INIT)
  target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE MUTEX_win32)
endif()

if(OPJ_USE_THREAD AND Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  target_compile_definitions(${OPENJPEG_LIBRARY_NAME} PRIVATE MUTEX_pthread)
endif()

if(OPJ_USE_THREAD AND NOT Threads_FOUND)
  message(FATAL_ERROR "No thread library found and thread/mutex support is required by OPJ_USE_THREAD option")
endif()

if(OPJ_USE_THREAD AND Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(${OPENJPEG_LIBRARY_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

