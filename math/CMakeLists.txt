########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_COMMS_MATH "Enable Pothos Comms.Math component" ON "ENABLE_COMMS" OFF)
add_feature_info("  Math" ENABLE_COMMS_MATH "Mathematical operators for sample streams")
if (NOT ENABLE_COMMS_MATH)
    return()
endif()

########################################################################
# Math blocks module
########################################################################
if(MSVC)
    add_definitions(/bigobj) # Some files too large in debug builds
endif(MSVC)

set(sources
    Abs.cpp
    Angle.cpp
    TestAngle.cpp
    Arithmetic.cpp
    TestComparatorBlocks.cpp
    Comparator.cpp
    Conjugate.cpp
    Log.cpp
    TestLog.cpp
    TestArithmeticBlocks.cpp
    Scale.cpp
    TestScale.cpp
    Rotate.cpp
    TestRotate.cpp
    ConstArithmetic.cpp
    Sinc.cpp
    TestSinc.cpp
    Trigonometric.cpp
    TestTrigonometric.cpp)
set(libraries CommsFunctions)

if(xsimd_FOUND)
    list(APPEND libraries xsimd)

    # /blocks/trigonometric (TODO: support multiple functions)
    pothos_multiarch(
        TrigonometricSIMDSources
        SIMD/Trigonometric.cpp
        PothosCommsSIMD
        "template <typename T> void cos(const T*, T*, size_t);"
        "cos"
        ${SIMDBuildArchs})

    set(SIMDSources ${TrigonometricSIMDSources})

    # Convert to relative path so PothosUtil will accept the path
    foreach(AbsPath ${SIMDSources})
        file(RELATIVE_PATH RelPath ${CMAKE_CURRENT_SOURCE_DIR} ${AbsPath})
        list(APPEND sources ${RelPath})
    endforeach()
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${JSON_HPP_INCLUDE_DIR})

POTHOS_MODULE_UTIL(
    TARGET MathBlocks
    SOURCES ${sources}
    LIBRARIES ${libraries}
    DESTINATION comms
    ENABLE_DOCS
)
