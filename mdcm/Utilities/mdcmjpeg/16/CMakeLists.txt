include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
add_library(${JPEG_LIBRARY_NAME}16 ${JPEG_SOURCES})
# Safely add NDEBUG
target_compile_definitions(${JPEG_LIBRARY_NAME}16 PRIVATE $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
set_target_properties(${JPEG_LIBRARY_NAME}16 PROPERTIES ${JPEG_LIBRARY_PROPERTIES})


