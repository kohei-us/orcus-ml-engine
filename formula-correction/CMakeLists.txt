cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW)

project(orcus-ml-engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS program_options filesystem)
find_package(PkgConfig REQUIRED)
find_package(Threads)
find_package(Python3 COMPONENTS Interpreter Development)

include(GNUInstallDirs)

if(Python3_FOUND)
    message(STATUS "python3 include dirs: ${Python3_INCLUDE_DIRS}")
    message(STATUS "python3 library dirs: ${Python3_LIBRARY_DIRS}")
    message(STATUS "python3 run-time library dirs: ${Python3_RUNTIME_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "python3 installation not found!")
endif()

pkg_check_modules(LIBMDDS REQUIRED mdds-2.0)
pkg_check_modules(LIBIXION REQUIRED libixion-0.17)
pkg_check_modules(LIBORCUS REQUIRED liborcus-0.17)

include_directories(
  ${LIBMDDS_INCLUDE_DIRS}
  ${LIBIXION_INCLUDE_DIRS}
  ${LIBORCUS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

add_subdirectory(src)
