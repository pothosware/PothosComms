########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_COMMS_WINDOW "Enable Pothos Comms.Window component" ON "ENABLE_COMMS;Spuce_FOUND" OFF)
add_feature_info("  Window" ENABLE_COMMS_WINDOW "Window tap designer blocks")
if (NOT ENABLE_COMMS_WINDOW)
    return()
endif()

########################################################################
# Window blocks module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET WindowBlocks
    SOURCES
        WindowDesigner.cpp
    DESTINATION comms
    LIBRARIES
        spuce
        CommsFunctions
    ENABLE_DOCS
)
