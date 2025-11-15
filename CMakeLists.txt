cmake_minimum_required(VERSION 3.11)
project(Hierarchy-Clear-GGCE)

# Project version and configuration
set(PROJECT_VERSION "2.0.0")
set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# Configuration options
option(BUILD_WITH_GGCE "Build with Global Graph Connectivity Enhancement" ON)
option(BUILD_TESTS "Build GGCE unit tests" ON)
option(BUILD_BENCHMARKS "Build GGCE performance benchmarks" OFF)
option(ENABLE_DEBUG_LOGGING "Enable debug logging in GGCE" OFF)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific settings
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Find required dependencies
find_package(Ceres CONFIG REQUIRED)
if(NOT Ceres_FOUND)
    message(FATAL_ERROR "Ceres not found - required for SFS reconstruction")
endif()

find_package(PCL CONFIG REQUIRED)
find_package(PCL CONFIG REQUIRED COMPONENTS common io)
if(NOT PCL_FOUND)
    message(FATAL_ERROR "PCL not found - required for point cloud processing")
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - parallel processing enabled")
else()
    message(WARNING "OpenMP not found - GGCE will run single-threaded")
endif()

# Find Eigen (usually comes with PCL but explicit check is better)
find_package(Eigen3 QUIET)
if(NOT Eigen3_FOUND)
    message(STATUS "Eigen3 not found via package manager, checking PCL bundled version")
endif()

# Find/Download TEASER++ for robust point cloud registration
include(FetchContent)

message(STATUS "Configuring TEASER++ (robust registration library)...")

FetchContent_Declare(
    teaserpp
    GIT_REPOSITORY https://github.com/MIT-SPARK/TEASER-plusplus.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# Configure TEASER++ build options (disable unnecessary components)
set(BUILD_TESTS OFF CACHE BOOL "Build TEASER++ tests" FORCE)
set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "Build Python bindings" FORCE)
set(BUILD_DOC OFF CACHE BOOL "Build documentation" FORCE)
set(BUILD_TEASER_FPFH OFF CACHE BOOL "Build FPFH features" FORCE)

FetchContent_MakeAvailable(teaserpp)

if(NOT teaserpp_POPULATED)
    message(FATAL_ERROR "TEASER++ download failed - required for robust rim assembly")
else()
    message(STATUS "✓ TEASER++ downloaded and configured successfully")
endif()

# Define source files
set(CORE_SOURCES
    class/data_structure.cpp
    class/feature_matching.cpp
    class/ranking_system.cpp
    class/reconstruction.cpp
    class/robust_icp.cpp
    class/filter.cpp
    class/visualize.cpp
    class/axis_estimation.cpp
    class/connectivity_optimizer.cpp
    class/intersection_detector.cpp
    class/KDTree.cpp
    class/hub_guided_beam_search.cpp
    class/puzzlefusion_global_optimizer.cpp
    class/rim_base_evidence.cpp
    class/surface_overlap_detector.cpp
    class/multi_hypothesis_optimizer.cpp
    class/physics_based_minimal_test.cpp
    class/full_physics_optimizer.cpp
    class/pottery_structure.cpp
    pottery_geometric_validator_simple.cpp
)

set(CORE_HEADERS
    class/data_structure.h
    class/feature_matching.h
    class/ranking_system.h
    class/reconstruction.h
    class/robust_icp.h
    class/filter.h
    class/visualize.h
    class/axis_estimation.h
    class/connectivity_optimizer.h
    class/intersection_detector.h
    class/KDTree.h
    class/hub_guided_beam_search.h
    class/puzzlefusion_global_optimizer.h
    class/rim_base_evidence.h
    class/surface_overlap_detector.h
    class/multi_hypothesis_optimizer.h
    class/physics_based_minimal_test.h
    class/full_physics_optimizer.h
    class/pottery_structure.h
    pottery_geometric_validator_simple.h
)

# GGCE-specific sources (only included if BUILD_WITH_GGCE is ON)
set(GGCE_SOURCES
    class/global_connectivity_engine.cpp
    class/global_connectivity_engine_part2.cpp
    class/enhanced_ranking_system.cpp
    class/two_phase_assembly.cpp
)

set(GGCE_HEADERS
    class/global_connectivity_engine.h
    class/enhanced_ranking_system.h
    class/two_phase_assembly.h
)

# Test sources (only included if BUILD_TESTS is ON)
set(TEST_SOURCES
    tests/test_ggce.cpp
)

set(TEST_HEADERS
    tests/test_ggce.h
)

# Main executable sources
set(MAIN_SOURCES
    main_headless_correct.cpp
)

# Create main library (always built)
add_library(hierarchy_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(hierarchy_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/class
        ${PCL_INCLUDE_DIRS}
        ${teaserpp_SOURCE_DIR}/teaser/include
)

target_link_libraries(hierarchy_core
    PUBLIC
        ${CERES_LIBRARIES}
        ${PCL_LIBRARIES}
        teaserpp::teaser_registration
        teaserpp::teaser_io
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(hierarchy_core PUBLIC OpenMP::OpenMP_CXX)
endif()

target_compile_definitions(hierarchy_core
    PUBLIC
        ${PCL_DEFINITIONS}
)

# GGCE library (conditionally built)
if(BUILD_WITH_GGCE)
    message(STATUS "Building with GGCE support")

    add_library(ggce_lib STATIC ${GGCE_SOURCES} ${GGCE_HEADERS})

    target_include_directories(ggce_lib
        PUBLIC
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/class
            ${PCL_INCLUDE_DIRS}
    )

    target_link_libraries(ggce_lib
        PUBLIC
            hierarchy_core
            ${CERES_LIBRARIES}
            ${PCL_LIBRARIES}
    )

    if(OpenMP_CXX_FOUND)
        target_link_libraries(ggce_lib PUBLIC OpenMP::OpenMP_CXX)
    endif()

    # Add GGCE-specific compile definitions
    target_compile_definitions(ggce_lib
        PUBLIC
            GGCE_ENABLED=1
            ${PCL_DEFINITIONS}
    )

    if(ENABLE_DEBUG_LOGGING)
        target_compile_definitions(ggce_lib PUBLIC GGCE_DEBUG_ENABLED=1)
    endif()

else()
    message(STATUS "Building without GGCE support")
    add_library(ggce_lib INTERFACE)  # Empty interface library for compatibility
endif()

# Main executable
add_executable(Hierarchy-Clear ${MAIN_SOURCES})

target_link_libraries(Hierarchy-Clear
    PRIVATE
        hierarchy_core
        ggce_lib
)

target_include_directories(Hierarchy-Clear
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/class
)

# Backward compatibility executable (original name without GGCE)
add_executable(Hierarchy-Clear-Original ${MAIN_SOURCES})

target_link_libraries(Hierarchy-Clear-Original
    PRIVATE
        hierarchy_core
)

target_include_directories(Hierarchy-Clear-Original
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/class
)

target_compile_definitions(Hierarchy-Clear-Original
    PRIVATE
        GGCE_DISABLED=1
)

# TEASER++ verification test
add_executable(test_teaser test_teaser.cpp)
target_link_libraries(test_teaser
    PRIVATE
        teaserpp::teaser_registration
)
message(STATUS "TEASER++ test executable will be built: ./test_teaser")

# Test executable (conditionally built)
if(BUILD_TESTS AND BUILD_WITH_GGCE)
    message(STATUS "Building GGCE unit tests")

    add_executable(ggce_tests ${TEST_SOURCES} ${TEST_HEADERS})

    target_link_libraries(ggce_tests
        PRIVATE
            hierarchy_core
            ggce_lib
    )

    target_include_directories(ggce_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/class
            ${CMAKE_SOURCE_DIR}/tests
    )

    # Add test-specific definitions
    target_compile_definitions(ggce_tests
        PRIVATE
            GGCE_TESTING=1
    )

    # Register tests with CTest
    enable_testing()
    add_test(
        NAME GGCEUnitTests
        COMMAND ggce_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Test timeout (5 minutes)
    set_tests_properties(GGCEUnitTests PROPERTIES TIMEOUT 300)

else()
    message(STATUS "Unit tests disabled (BUILD_TESTS=${BUILD_TESTS}, BUILD_WITH_GGCE=${BUILD_WITH_GGCE})")
endif()

# Benchmark executable (conditionally built)
if(BUILD_BENCHMARKS AND BUILD_WITH_GGCE)
    message(STATUS "Building GGCE benchmarks")

    add_executable(ggce_benchmarks tests/benchmark_ggce.cpp)

    target_link_libraries(ggce_benchmarks
        PRIVATE
            hierarchy_core
            ggce_lib
    )

    target_include_directories(ggce_benchmarks
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/class
            ${CMAKE_SOURCE_DIR}/tests
    )

    target_compile_definitions(ggce_benchmarks
        PRIVATE
            GGCE_BENCHMARKING=1
    )
endif()

# Installation configuration
install(TARGETS Hierarchy-Clear Hierarchy-Clear-Original
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

if(BUILD_WITH_GGCE AND BUILD_TESTS)
    install(TARGETS ggce_tests
        RUNTIME DESTINATION bin
        COMPONENT Testing
    )
endif()

if(BUILD_WITH_GGCE AND BUILD_BENCHMARKS)
    install(TARGETS ggce_benchmarks
        RUNTIME DESTINATION bin
        COMPONENT Benchmarking
    )
endif()

# Install headers for development
install(FILES ${CORE_HEADERS}
    DESTINATION include/hierarchy_core
    COMPONENT Development
)

if(BUILD_WITH_GGCE)
    install(FILES ${GGCE_HEADERS}
        DESTINATION include/ggce
        COMPONENT Development
    )
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "Hierarchy-Clear-GGCE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SFS Reconstruction with Global Graph Connectivity Enhancement")
set(CPACK_PACKAGE_VENDOR "University of Melbourne")
set(CPACK_PACKAGE_CONTACT "SFS Development Team")

# Component descriptions
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Binaries")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable files for SFS reconstruction")

set(CPACK_COMPONENT_TESTING_DISPLAY_NAME "Test Suite")
set(CPACK_COMPONENT_TESTING_DESCRIPTION "GGCE unit tests and validation tools")

set(CPACK_COMPONENT_BENCHMARKING_DISPLAY_NAME "Performance Benchmarks")
set(CPACK_COMPONENT_BENCHMARKING_DESCRIPTION "GGCE performance benchmarking tools")

set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files for development and extension")

include(CPack)

# Configuration summary
message(STATUS "")
message(STATUS "===== Build Configuration Summary =====")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "GGCE Support: ${BUILD_WITH_GGCE}")
message(STATUS "Unit Tests: ${BUILD_TESTS}")
message(STATUS "Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "Debug Logging: ${ENABLE_DEBUG_LOGGING}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  Ceres: ${Ceres_VERSION}")
message(STATUS "  PCL: ${PCL_VERSION}")
if(Eigen3_FOUND)
    message(STATUS "  Eigen3: ${Eigen3_VERSION}")
endif()
message(STATUS "")
message(STATUS "Targets:")
message(STATUS "  Hierarchy-Clear (with GGCE)")
message(STATUS "  Hierarchy-Clear-Original (without GGCE)")
if(BUILD_TESTS AND BUILD_WITH_GGCE)
    message(STATUS "  ggce_tests")
endif()
if(BUILD_BENCHMARKS AND BUILD_WITH_GGCE)
    message(STATUS "  ggce_benchmarks")
endif()
message(STATUS "=======================================")
message(STATUS "")

# Build commands help
message(STATUS "Build Commands:")
message(STATUS "  make all                    # Build all targets")
message(STATUS "  make Hierarchy-Clear        # Build enhanced version")
message(STATUS "  make Hierarchy-Clear-Original  # Build original version")
if(BUILD_TESTS AND BUILD_WITH_GGCE)
    message(STATUS "  make ggce_tests            # Build tests")
    message(STATUS "  make test                  # Run tests")
endif()
if(BUILD_BENCHMARKS AND BUILD_WITH_GGCE)
    message(STATUS "  make ggce_benchmarks       # Build benchmarks")
endif()
message(STATUS "  make install               # Install binaries")
message(STATUS "  make package               # Create distribution package")
message(STATUS "")

# Environment variable documentation
if(BUILD_WITH_GGCE)
    message(STATUS "GGCE Environment Variables:")
    message(STATUS "  GGCE_ENABLED=1             # Enable GGCE at runtime")
    message(STATUS "  GGCE_DEBUG=1               # Enable debug output")
    message(STATUS "  GGCE_VERBOSE=1             # Enable verbose logging")
    message(STATUS "  GGCE_CONNECTIVITY_WEIGHT=0.6  # Algorithm parameters")
    message(STATUS "  GGCE_QUALITY_WEIGHT=0.3")
    message(STATUS "  GGCE_CONSISTENCY_WEIGHT=0.1")
    message(STATUS "  GGCE_VARIANT=balanced      # Algorithm variant")
    message(STATUS "")
endif()