set(JPEG_NAMESPACE "MDCMJPEG")
add_subdirectory(mdcmjpeg)

if(NOT MDCM_USE_SYSTEM_OPENJPEG)
  set(OPENJPEG_NAMESPACE "MDCMOPENJPEG")
  add_subdirectory(mdcmopenjpeg)
endif()

if(NOT MDCM_USE_SYSTEM_CHARLS)
  set(CHARLS_NAMESPACE "MDCMCHARLS")
  add_subdirectory(mdcmcharls)
endif()

if(NOT MDCM_USE_SYSTEM_ZLIB)
  set(ZLIB_NAMESPACE "MDCMZLIB")
  add_subdirectory(mdcmzlib)
endif()

if(NOT WIN32 AND NOT MINGW)
  if(NOT MDCM_USE_SYSTEM_UUID)
    set(UUID_NAMESPACE "MDCMUUID")
    add_subdirectory(mdcmuuid)
  endif()
endif()
