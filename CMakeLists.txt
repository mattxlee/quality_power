cmake_minimum_required(VERSION 2.7)

project(quality_power)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-o0 -g")

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED program_options)

find_library(GMP_LIBRARY gmp)
find_library(MPFR_LIBRARY mpfr)

include_directories(${OPENSSL_INCLUDE_DIR})

file(GLOB QP_SRC "src/quality_power.cc")

add_executable(quality_power ${QP_SRC})

target_link_libraries(quality_power
  ${GMP_LIBRARY}
  ${MPFR_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
  )
