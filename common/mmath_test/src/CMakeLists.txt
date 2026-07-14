cmake_minimum_required(VERSION 3.10)
project(MMathAlmostEqualTests LANGUAGES CXX)

# Allow overriding the implementation file (default mmath_impl.cpp)
set(MMATH_IMPL "../../mmath.cpp" CACHE STRING "Path to your MMath implementation source file")

# Test source
set(TEST_SRC test_almost_equal.cpp)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_almost_equal ${TEST_SRC} ${MMATH_IMPL})

# Recommended warnings
if (MSVC)
    target_compile_options(test_almost_equal PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_almost_equal PRIVATE -Wall -Wextra -Wpedantic)
    # Link math library explicitly on some toolchains if needed (usually not needed for C++)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(test_almost_equal PRIVATE ${M_LIB})
    endif()
endif()

# Install target for convenience (optional)
install(TARGETS test_almost_equal RUNTIME DESTINATION bin)
