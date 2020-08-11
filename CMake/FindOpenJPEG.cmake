find_package(PkgConfig)
pkg_check_modules(OPENJPEG libopenjp2)

if(OPENJPEG_FOUND)
  message(STATUS "Found OpenJPEG ${OPENJPEG_VERSION} libs: ${OPENJPEG_LIBRARIES}, incl: ${OPENJPEG_INCLUDE_DIRS}")
else()
  message(STATUS "OpenJPEG not found")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenJPEG REQUIRED_VARS
 OPENJPEG_LIBRARIES
 OPENJPEG_INCLUDE_DIRS
 VERSION_VAR OPENJPEG_VERSION)

mark_as_advanced(OPENJPEG_LIBRARIES OPENJPEG_INCLUDE_DIRS)
