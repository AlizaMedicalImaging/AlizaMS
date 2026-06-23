add_library(${JPEG_LIBRARY_NAME}8 ${JPEG_SOURCES})
# Safely add NDEBUG
target_include_directories(${JPEG_LIBRARY_NAME}8 BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(${JPEG_LIBRARY_NAME}8 PRIVATE $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
set_target_properties(${JPEG_LIBRARY_NAME}8 PROPERTIES ${JPEG_LIBRARY_PROPERTIES})

