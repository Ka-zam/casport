#######################################################################################################
#
#  CMake file for libcasport
#    >mkdir build && cd build;cmake ..;make -j8
#
#
#######################################################################################################

cmake_minimum_required(VERSION 3.14..3.22)
project(casport VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE "OFF")

# Build preferences
set(DEFAULT_BUILD_TYPE "Release")
option(BUILD_SHARED_LIBS "Build shared libs." ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#######################################################################################################
#
# External dependencies
#
#######################################################################################################

#######################################################################################################
#
#  Add to our project
#
#######################################################################################################

set(SOURCE_FILES 
 ${CMAKE_CURRENT_SOURCE_DIR}/src/casport.cc
)
 
add_library(${PROJECT_NAME} SHARED
    ${SOURCE_FILES})
    
target_compile_options(${PROJECT_NAME} PUBLIC -Wall )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
   
add_subdirectory(apps)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1)

install(TARGETS ${PROJECT_NAME} EXPORT casportConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(EXPORT casportConfig DESTINATION share/casport/cmake)

export(TARGETS ${PROJECT_NAME} FILE casportConfig.cmake)
