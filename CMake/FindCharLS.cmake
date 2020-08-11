#TODO
find_path(CHARLS_INCLUDE_DIR CharLS/charls.h /usr/include /usr/local/include)
if (NOT CHARLS_INCLUDE_DIR)
  find_path(CHARLS_INCLUDE_DIR charls/charls.h /usr/include /usr/local/include)
endif()

find_library(CHARLS_LIBRARY NAMES CharLS PATHS /usr/lib /usr/local/lib)
if (NOT CHARLS_LIBRARY)
  find_library(CHARLS_LIBRARY NAMES charls PATHS /usr/lib /usr/local/lib)
endif()

if(CHARLS_LIBRARY AND CHARLS_INCLUDE_DIR)
  set(CHARLS_LIBRARIES    ${CHARLS_LIBRARY})
  set(CHARLS_INCLUDE_DIRS ${CHARLS_INCLUDE_DIR})
  set(CHARLS_FOUND "YES")
else()
  set(CHARLS_FOUND "NO")
endif()

if(CHARLS_FOUND)
  message(STATUS "Found CHARLS lib: ${CHARLS_LIBRARIES}, incl: ${CHARLS_INCLUDE_DIRS}")
else()
  message(FATAL_ERROR "CHARLS not found")
endif()

mark_as_advanced(CHARLS_LIBRARY CHARLS_INCLUDE_DIR)
