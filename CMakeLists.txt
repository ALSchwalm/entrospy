project(entrospy)

cmake_minimum_required(VERSION 2.6)
find_package(Boost REQUIRED COMPONENTS program_options system filesystem)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Unable to locate boost libraries")
else()
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 ${CMAKE_CXX_FLAGS}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

  include_directories(${Boost_INCLUDE_DIRS} "entrospy/include")
  file(GLOB SOURCES "entrospy/*.cpp")

  add_executable(entrospy ${SOURCES})
  target_link_libraries(entrospy ${Boost_LIBRARIES})
endif()
