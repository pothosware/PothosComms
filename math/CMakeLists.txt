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
    TestAbs.cpp
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

    # /blocks/abs
    pothos_multiarch(
        AbsSIMDSources
        SIMD/Abs.cpp
        PothosCommsSIMD
        "template <typename T> void abs(const T*, T*, size_t);"
        "abs"
        ${SIMDBuildArchs})

    # /blocks/arithmetic (TODO: support multiple functions)
    pothos_multiarch(
        ArithmeticSIMDSources
        SIMD/Arithmetic.cpp
        PothosCommsSIMD
        "template <typename T> void add(const T*, const T*, T*, size_t);"
        "add"
        ${SIMDBuildArchs})

    # /blocks/const_arithmetic (TODO: support multiple functions)
    pothos_multiarch(
        ConstArithmeticSIMDSources
        SIMD/ConstArithmetic.cpp
        PothosCommsSIMD
        "template <typename T> void XPlusK(const T*, const T&, T*, size_t);"
        "XPlusK"
        ${SIMDBuildArchs})

    # /blocks/log* (TODO: support multiple functions)
    pothos_multiarch(
        LogSIMDSources
        SIMD/Log.cpp
        PothosCommsSIMD
        "template <typename T> void log2(const T*, T*, size_t);"
        "log2"
        ${SIMDBuildArchs})
        
    pothos_multiarch(
        SincSIMDSources
        SIMD/Sinc.cpp
        PothosCommsSIMD
        "template <typename T> void sinc(const T*, T*, size_t);"
        "sinc"
        ${SIMDBuildArchs})

    # /blocks/trigonometric (TODO: support multiple functions)
    pothos_multiarch(
        TrigonometricSIMDSources
        SIMD/Trigonometric.cpp
        PothosCommsSIMD
        "template <typename T> void cos(const T*, T*, size_t);"
        "cos"
        ${SIMDBuildArchs})

    set(SIMDSources
        ${AbsSIMDSources}
        ${ArithmeticSIMDSources}
        ${ConstArithmeticSIMDSources}
        ${LogSIMDSources}
        ${SincSIMDSources}
        ${TrigonometricSIMDSources})

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
