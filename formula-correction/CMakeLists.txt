cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW)

project(orcus-ml-engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMDDS REQUIRED mdds-1.5)
pkg_check_modules(LIBORCUS REQUIRED liborcus-0.16)

include_directories(
  ${LIBMDDS_INCLUDE_DIRS}
  ${LIBORCUS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

add_subdirectory(src)