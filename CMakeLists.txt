########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosComms CXX C)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    find_package(Pothos "0.6.0" CONFIG REQUIRED)
else()
    find_package(Pothos CONFIG REQUIRED) #in-tree build
endif()

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_COMMS "Enable Pothos Comms component" ON "Pothos_FOUND" OFF)
add_feature_info(Comms ENABLE_COMMS "A collection of DSP/communications blocks")

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "Pothos Comms toolkit requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
# Spuce library
########################################################################
if(NOT SPUCE_IN_TREE)
    find_package(Spuce CONFIG)
endif(NOT SPUCE_IN_TREE)

if (Spuce_FOUND)
    message(STATUS "Spuce_VERSION: ${Spuce_VERSION}")
    message(STATUS "Spuce_INCLUDE_DIRS: ${Spuce_INCLUDE_DIRS}")
    message(STATUS "Spuce_LIBRARIES: ${Spuce_LIBRARIES}")
else (Spuce_FOUND)
    message(WARNING "Spuce filter designer library not found...")
endif (Spuce_FOUND)

########################################################################
# Build subdirectories
########################################################################
include_directories(functions)
add_subdirectory(functions)
add_subdirectory(demod)
add_subdirectory(digital)
add_subdirectory(fft)
add_subdirectory(filter)
add_subdirectory(mac)
add_subdirectory(math)
add_subdirectory(utility)
add_subdirectory(waveform)
