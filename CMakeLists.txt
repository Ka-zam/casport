cmake_minimum_required(VERSION 3.14)
project(cascadix VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_WASM "Build WebAssembly target" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler flags
if(NOT EMSCRIPTEN)
    # Native build flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
else()
    # Emscripten/WASM flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/cascadix/include)

# Main library sources
set(CASCADIX_SOURCES
    cascadix/src/two_port.cc
    cascadix/src/components.cc
    cascadix/src/smith_chart_generator.cc
    cascadix/src/component_sweep.cc
    cascadix/src/smith_chart.cc
    cascadix/src/monte_carlo.cc
)

# Create library
if(BUILD_SHARED_LIBS AND NOT EMSCRIPTEN)
    add_library(${PROJECT_NAME} SHARED ${CASCADIX_SOURCES})
else()
    add_library(${PROJECT_NAME} STATIC ${CASCADIX_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/cascadix/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Build tests if requested and not building for WASM
if(BUILD_TESTS AND NOT EMSCRIPTEN)
    # Download and configure GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    
    # Test executables
    add_executable(test_components cascadix/tests/test_components.cc)
    target_link_libraries(test_components ${PROJECT_NAME} gtest_main)
    
    add_executable(test_cascading cascadix/tests/test_cascading.cc)
    target_link_libraries(test_cascading ${PROJECT_NAME} gtest_main)
    
    add_executable(test_conversions cascadix/tests/test_conversions.cc)
    target_link_libraries(test_conversions ${PROJECT_NAME} gtest_main)
    
    add_executable(test_frequency_sweep cascadix/tests/test_frequency_sweep.cc)
    target_link_libraries(test_frequency_sweep ${PROJECT_NAME} gtest_main)
    
    add_executable(test_smith_chart_generator cascadix/tests/test_smith_chart_generator.cc)
    target_link_libraries(test_smith_chart_generator ${PROJECT_NAME} gtest_main)
    
    # Add tests
    include(GoogleTest)
    gtest_discover_tests(test_components)
    gtest_discover_tests(test_cascading)
    gtest_discover_tests(test_conversions)
    gtest_discover_tests(test_frequency_sweep)
    gtest_discover_tests(test_smith_chart_generator)
    
    # Combined test runner
    add_custom_target(run_tests
        COMMAND test_components
        COMMAND test_cascading
        COMMAND test_conversions
        COMMAND test_frequency_sweep
        COMMAND test_smith_chart_generator
        DEPENDS test_components test_cascading test_conversions test_frequency_sweep test_smith_chart_generator
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Build examples if requested
if(BUILD_EXAMPLES AND NOT EMSCRIPTEN)
    add_subdirectory(cascadix/apps)
endif()

# WebAssembly specific configuration
if(EMSCRIPTEN)
    # Set output files
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wasm
    )
    
    # Add WASM bindings if they exist
    if(EXISTS ${CMAKE_SOURCE_DIR}/cascadix/wasm/bindings.cc)
        add_executable(cascadix_wasm cascadix/wasm/bindings.cc)
        target_link_libraries(cascadix_wasm ${PROJECT_NAME})
        
        set_target_properties(cascadix_wasm PROPERTIES
            SUFFIX ".js"
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wasm
            LINK_FLAGS "-s MODULARIZE=1 -s EXPORT_NAME='CascadixModule' -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] --bind"
        )
        
        # Copy HTML demo file to build directory if it exists
        if(EXISTS ${CMAKE_SOURCE_DIR}/cascadix/wasm/demo.html)
            configure_file(${CMAKE_SOURCE_DIR}/cascadix/wasm/demo.html 
                          ${CMAKE_BINARY_DIR}/wasm/index.html COPYONLY)
        endif()
    endif()
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT cascadixConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY cascadix/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(EXPORT cascadixConfig DESTINATION share/cascadix/cmake)

# Export for use in build tree
export(TARGETS ${PROJECT_NAME} FILE cascadixConfig.cmake)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cascadixConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Print configuration summary
message(STATUS "")
message(STATUS "cascadix configuration summary:")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests:    ${BUILD_TESTS}")
message(STATUS "  Build WASM:     ${BUILD_WASM}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Shared libs:    ${BUILD_SHARED_LIBS}")
if(EMSCRIPTEN)
    message(STATUS "  Target:         WebAssembly")
else()
    message(STATUS "  Target:         Native")
endif()
message(STATUS "")