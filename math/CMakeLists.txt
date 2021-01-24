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
    add_definitions(/bigobj) #arithmetic too large in debug mode
endif(MSVC)

set(libraries CommsFunctions)

if(xsimd_FOUND)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})
    add_subdirectory(SIMD)
    list(APPEND libraries CommsMathSIMD)
endif()

POTHOS_MODULE_UTIL(
    TARGET MathBlocks
    SOURCES
        Abs.cpp
        TestAbs.cpp
        Angle.cpp
        TestAngle.cpp
        Arithmetic.cpp
        TestComparatorBlocks.cpp
        Comparator.cpp
        ConstComparator.cpp
        Conjugate.cpp
        TestConjugate.cpp
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
        TestTrigonometric.cpp
        Pow.cpp
        Root.cpp
        TestPowRoot.cpp
        RSqrt.cpp
        TestRSqrt.cpp
        Beta.cpp
        Gamma.cpp
        TestBeta.cpp
        TestGamma.cpp
    LIBRARIES ${libraries}
    DESTINATION comms
    ENABLE_DOCS
)
