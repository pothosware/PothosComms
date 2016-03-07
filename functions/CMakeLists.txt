########################################################################
## Comms function library
########################################################################
set(SOURCES
    fxpt_atan2.cpp
)

include_directories(${Pothos_INCLUDE_DIRS})
add_library(CommsFunctions STATIC ${SOURCES})
set_property(TARGET CommsFunctions PROPERTY POSITION_INDEPENDENT_CODE TRUE)
