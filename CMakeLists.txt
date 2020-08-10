####################################################################
#
# Aliza MS
#
####################################################################
cmake_minimum_required(VERSION 3.11.0)
project(ALIZAMS)

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

find_package(MDCM 1.0.28 REQUIRED)
include(${MDCM_USE_FILE})
message(STATUS "MDCM: ${MDCM_LIBRARIES}")

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
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  add_definitions(-DDISABLE_SIMDMATH)
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
else()
  if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    link_directories(/usr/local/lib)
  endif()
endif()

set(ALIZAMS_LINK_LIBRARIES ${MDCM_LIBRARIES} ${ITK_LIBRARIES})

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
  target_link_libraries(alizams ${ALIZAMS_LINK_LIBRARIES})
endif()

install(TARGETS alizams RUNTIME DESTINATION "bin")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/icons" DESTINATION "share")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/package/archive/usr/share/applications" DESTINATION "share")

