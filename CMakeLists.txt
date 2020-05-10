cmake_minimum_required(VERSION 3.5)

# set the project name
project(libusmc)


find_package(PkgConfig REQUIRED)

# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib/")
include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCE_FILES
    src/libusmc.cpp
    src/libusmc_impl.cpp
    src/usmc_mutex.cpp
)

# add library
add_library(usmc SHARED ${SOURCE_FILES})
target_link_libraries(usmc PkgConfig::LIBUSB)

# test program
add_executable(usmc_test src/usmc_test.cpp)
target_link_libraries(usmc_test usmc)

# Install rules
