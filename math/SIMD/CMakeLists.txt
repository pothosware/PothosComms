########################################################################
## Make a static library with the SIMD implementations because MSVC
## doesn't like MathBlocksDocs.cpp depending on too many things.
########################################################################

set(SIMDInputs
    Abs.cpp
    Arithmetic.cpp
    Comparator.cpp
    Conjugate.cpp
    ConstArithmetic.cpp
    Log.cpp
    Sinc.cpp
    Trigonometric.cpp)

PothosGenerateSIMDSources(
    SIMDSources
    MathBlocks.json
    ${SIMDInputs})

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Pothos_INCLUDE_DIRS})

set(libraries
    ${Pothos_LIBRARIES}
    xsimd)

add_library(CommsMathSIMD STATIC ${SIMDSources})
target_link_libraries(CommsMathSIMD ${libraries})
add_dependencies(CommsMathSIMD MathBlocks_SIMDDispatcher)
set_property(TARGET CommsMathSIMD PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# This library is pure templates, so expect large object files.
if(MSVC)
    set_property(TARGET CommsMathSIMD PROPERTY COMPILE_FLAGS /bigobj)
endif()
