include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
add_library(${JPEG_LIBRARY_NAME}8 ${JPEG_SOURCES})
# Safely add NDEBUG
target_compile_definitions(
  ${JPEG_LIBRARY_NAME}8 PRIVATE $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:NDEBUG>)
set_target_properties(${JPEG_LIBRARY_NAME}8 PROPERTIES ${JPEG_LIBRARY_PROPERTIES})

