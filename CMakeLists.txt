# Define the minimum version of CMake required to build the project
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(HashTable VERSION 1.0)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add the library
add_library(hashtable ${PROJECT_SOURCE_DIR}/src/hashtable.c)

# Add the test executable
add_executable(hashtable_test ${PROJECT_SOURCE_DIR}/tests/unit_tests.cpp)

# Specify binaries output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Specify libraries output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# Find Google Test Package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Enable AddressSanitizer
# Enable AddressSanitizer
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# Link the library with the test executable
target_link_libraries(hashtable_test hashtable ${GTEST_LIBRARIES} pthread)

# Enable testing
enable_testing()

# Add the test
add_test(NAME hashtable_test COMMAND hashtable_test)
