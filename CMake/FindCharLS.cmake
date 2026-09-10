find_path(CHARLS_INCLUDE_DIR charls.h PATHS /usr/include/charls /usr/local/include/charls /usr/include/CharLS /usr/local/include/CharLS)
find_library(CHARLS_LIBRARY NAMES charls CharLS PATHS /usr/lib /usr/local/lib)

if(CHARLS_INCLUDE_DIR AND CHARLS_LIBRARY)
  set(CHARLS_FOUND "YES")
else()
  set(CHARLS_FOUND "NO")
endif()

if(CHARLS_FOUND)
  message(STATUS "Found CharLS library: ${CHARLS_LIBRARY}, incl: ${CHARLS_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "CharLS not found")
endif()
