########################################################################
## Make a static library with the SIMD implementations because MSVC
## doesn't like MathBlocksDocs.cpp depending on too many things.
########################################################################

# /blocks/abs
pothos_multiarch(
    AbsSIMDSources
    Abs.cpp
    PothosCommsSIMD
    "template <typename T> void abs(const T*, T*, size_t);"
    "abs"
    ${SIMDBuildArchs})

# /blocks/arithmetic (TODO: support multiple functions)
pothos_multiarch(
    ArithmeticSIMDSources
    Arithmetic.cpp
    PothosCommsSIMD
    "template <typename T> void add(const T*, const T*, T*, size_t);"
    "add"
    ${SIMDBuildArchs})

# /blocks/comparator (TODO: support multiple functions)
pothos_multiarch(
    ComparatorSIMDSources
    Comparator.cpp
    PothosCommsSIMD
    "template <typename T> void greaterThan(const T*, const T*, char*, const size_t);"
    "greaterThan"
    ${SIMDBuildArchs})

# /blocks/conjugate
pothos_multiarch(
    ConjugateSIMDSources
    Conjugate.cpp
    PothosCommsSIMD
    "template <typename T> void conj(const T*, T*, size_t);"
    "conj"
    ${SIMDBuildArchs})

# /blocks/const_arithmetic (TODO: support multiple functions)
pothos_multiarch(
    ConstArithmeticSIMDSources
    ConstArithmetic.cpp
    PothosCommsSIMD
    "template <typename T> void XPlusK(const T*, const T&, T*, size_t);"
    "XPlusK"
    ${SIMDBuildArchs})

# /blocks/log* (TODO: support multiple functions)
pothos_multiarch(
    LogSIMDSources
    Log.cpp
    PothosCommsSIMD
    "template <typename T> void log2(const T*, T*, size_t);"
    "log2"
    ${SIMDBuildArchs})
    
pothos_multiarch(
    SincSIMDSources
    Sinc.cpp
    PothosCommsSIMD
    "template <typename T> void sinc(const T*, T*, size_t);"
    "sinc"
    ${SIMDBuildArchs})

# /blocks/trigonometric (TODO: support multiple functions)
pothos_multiarch(
    TrigonometricSIMDSources
    Trigonometric.cpp
    PothosCommsSIMD
    "template <typename T> void cos(const T*, T*, size_t);"
    "cos"
    ${SIMDBuildArchs})

set(sources
    ${AbsSIMDSources}
    ${ArithmeticSIMDSources}
    ${ComparatorSIMDSources}
    ${ConjugateSIMDSources}
    ${ConstArithmeticSIMDSources}
    ${LogSIMDSources}
    ${SincSIMDSources}
    ${TrigonometricSIMDSources})
    
include_directories(${Pothos_INCLUDE_DIRS})

set(libraries
    ${Pothos_LIBRARIES}
    xsimd)
    
add_library(CommsMathSIMD STATIC ${sources})
target_link_libraries(CommsMathSIMD ${libraries})
set_property(TARGET CommsMathSIMD PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# This library is pure templates, so expect large object files.
if(MSVC)
    set_property(TARGET CommsMathSIMD PROPERTY COMPILE_FLAGS /bigobj)
endif()