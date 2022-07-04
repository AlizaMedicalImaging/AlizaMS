cmake_minimum_required(VERSION 3.11.0)

project(ALIZALCMS2 C)

set(BUILD_SHARED_LIBS OFF)

set(LCMS2_SRCS
  cmsalpha.c
  cmscam02.c
  cmscgats.c
  cmscnvrt.c
  cmserr.c
  cmsgamma.c
  cmsgmt.c
  cmshalf.c
  cmsintrp.c
  cmsio0.c
  cmsio1.c
  cmslut.c
  cmsmd5.c
  cmsmtrx.c
  cmsnamed.c
  cmsopt.c
  cmspack.c
  cmspcs.c
  cmsplugin.c
  cmsps2.c
  cmssamp.c
  cmssm.c
  cmstypes.c
  cmsvirt.c
  cmswtpnt.c
  cmsxform.c)

set(ALIZALCMS_LIBRARY_PROPERTIES VERSION "2.13.2")

include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(alizalcms2 ${LCMS2_SRCS})
set_target_properties(alizalcms2 PROPERTIES ${ALIZALCMS_LIBRARY_PROPERTIES})

