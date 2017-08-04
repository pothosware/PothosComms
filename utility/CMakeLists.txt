########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_COMMS_UTILITY "Enable Pothos Comms.Utility component" ON "ENABLE_COMMS;JSON_HPP_INCLUDE_DIR" OFF)
add_feature_info("  Utility" ENABLE_COMMS_UTILITY "Miscellaneous utilities for sample streams")
if (NOT ENABLE_COMMS_UTILITY)
    return()
endif()

########################################################################
# Utility blocks module
########################################################################
include_directories(${JSON_HPP_INCLUDE_DIR})
POTHOS_MODULE_UTIL(
    TARGET UtilityBlocks
    SOURCES
        SignalProbe.cpp
        Threshold.cpp
        WaveTrigger.cpp
        SplitComplex.cpp
        CombineComplex.cpp
        TestComplex.cpp
    DESTINATION comms
    ENABLE_DOCS
)
