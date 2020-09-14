// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/Trigonometric_SIMDDispatcher.hpp"
#endif

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>
#include <type_traits>

template <typename T>
using TrigFunc = void(*)(const T*, T*, size_t);

//
// Getters for functions, called on class construction
//

#ifdef POTHOS_XSIMD

template <typename T>
static TrigFunc<T> getCos()
{
    return PothosCommsSIMD::cosDispatch<T>();
}

#else

template <typename T>
static TrigFunc<T> getCos()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::cos(in[i]);
    };

    return impl;
}

#endif

// TODO: move into #else case when other SIMD implementations exposed

template <typename T>
static TrigFunc<T> getSin()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::sin(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getTan()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::tan(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getSec()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::cos(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getCsc()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::sin(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getCot()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::tan(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACos()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::acos(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getASin()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::asin(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getATan()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::atan(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getASec()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::acos(T(1.0) / in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACsc()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::asin(T(1.0) / in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACot()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::atan(T(1.0) / in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getCosH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::cosh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getSinH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::sinh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getTanH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::tanh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getSecH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::cosh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getCscH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::sinh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getCotH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(1.0) / std::tanh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACosH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::acosh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getASinH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::asinh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getATanH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::atanh(in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getASecH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::acosh(T(1.0) / in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACscH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::asinh(T(1.0) / in[i]);
    };

    return impl;
}

template <typename T>
static TrigFunc<T> getACotH()
{
    static const auto impl = [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::atanh(T(1.0) / in[i]);
    };

    return impl;
}

/***********************************************************************
 * |PothosDoc Trigonometric
 *
 * Perform trigonometric operations on all input elements.
 *
 * Available functions:
 * <ul>
 * <li><b>COS:</b> cosine</li>
 * <li><b>SIN:</b> sine</li>
 * <li><b>TAN:</b> tangent</li>
 * <li><b>SEC:</b> secant (1/cos(x))</li>
 * <li><b>CSC:</b> cosecant (1/sin(x))</li>
 * <li><b>COT:</b> cotangent (1/tan(x))</li>
 * <li><b>ACOS:</b> arc cosine</li>
 * <li><b>ASIN:</b> arc sine</li>
 * <li><b>ATAN:</b> arc tangent</li>
 * <li><b>ASEC:</b> arc secant (acos(1/x))</li>
 * <li><b>ACSC:</b> arc cosecant (asin(1/x))</li>
 * <li><b>ACOT:</b> arc cotangent (atan(1/x))</li>
 * <li><b>COSH:</b> hyperbolic cosine</li>
 * <li><b>SINH:</b> hyperbolic sine</li>
 * <li><b>TANH:</b> hyperbolic tangent</li>
 * <li><b>SECH:</b> hyperbolic secant (1/cosh(x))</li>
 * <li><b>CSCH:</b> hyperbolic cosecant (1/sinh(x))</li>
 * <li><b>COTH:</b> hyperbolic cotangent (1/tanh(x))</li>
 * <li><b>ACOSH:</b> hyperbolic arc cosine</li>
 * <li><b>ASINH:</b> hyperbolic arc sine</li>
 * <li><b>ATANH:</b> hyperbolic arc tangent</li>
 * <li><b>ASECH:</b> hyperbolic arc secant (acosh(1/x))</li>
 * <li><b>ACSCH:</b> hyperbolic arc cosecant (asinh(1/x))</li>
 * <li><b>ACOTH:</b> hyperbolic arc cotangent (atanh(1/x))</li>
 * </ul>
 *
 * |category /Math
 * |keywords cos sin tan sec csc cot
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |param operation The trigonometric function to perform.
 * |widget ComboBox(editable=false)
 * |default "COS"
 * |option [Cos] "COS"
 * |option [Sin] "SIN"
 * |option [Tan] "TAN"
 * |option [Sec] "SEC"
 * |option [Csc] "CSC"
 * |option [Cot] "COT"
 * |option [ArcCos] "ACOS"
 * |option [ArcSin] "ASIN"
 * |option [ArcTan] "ATAN"
 * |option [ArcSec] "ASEC"
 * |option [ArcCsc] "ACSC"
 * |option [ArcCot] "ACOT"
 * |option [CosH] "COSH"
 * |option [SinH] "SINH"
 * |option [TanH] "TANH"
 * |option [SecH] "SECH"
 * |option [CscH] "CSCH"
 * |option [CotH] "COTH"
 * |option [ArcCosH] "ACOSH"
 * |option [ArcSinH] "ASINH"
 * |option [ArcTanH] "ATANH"
 * |option [ArcSecH] "ASECH"
 * |option [ArcCscH] "ACSCH"
 * |option [ArcCotH] "ACOTH"
 *
 * |factory /comms/trigonometric(dtype, operation)
 * |initializer setOperation(operation)
 **********************************************************************/
template <typename T>
class Trigonometric: public Pothos::Block
{
public:
    using Func = TrigFunc<T>;
    using Class = Trigonometric<T>;

    Trigonometric(const std::string& operation, size_t dimension)
    {
        const Pothos::DType dtype(typeid(T), dimension);
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->setOperation(operation);

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setOperation));
    }

    void setOperation(const std::string& funcName)
    {
        #define ifNameSetFunc(name, getter) \
            if(name == funcName) _func = getter<T>();

        ifNameSetFunc     ("COS",   getCos)
        else ifNameSetFunc("SIN",   getSin)
        else ifNameSetFunc("TAN",   getTan)
        else ifNameSetFunc("SEC",   getSec)
        else ifNameSetFunc("CSC",   getCsc)
        else ifNameSetFunc("COT",   getCot)
        else ifNameSetFunc("ACOS",  getACos)
        else ifNameSetFunc("ASIN",  getASin)
        else ifNameSetFunc("ATAN",  getATan)
        else ifNameSetFunc("ASEC",  getASec)
        else ifNameSetFunc("ACSC",  getACsc)
        else ifNameSetFunc("ACOT",  getACot)
        else ifNameSetFunc("COSH",  getCosH)
        else ifNameSetFunc("SINH",  getSinH)
        else ifNameSetFunc("TANH",  getTanH)
        else ifNameSetFunc("SECH",  getSecH)
        else ifNameSetFunc("CSCH",  getCscH)
        else ifNameSetFunc("COTH",  getCotH)
        else ifNameSetFunc("ACOSH", getACosH)
        else ifNameSetFunc("ASINH", getASinH)
        else ifNameSetFunc("ATANH", getATanH)
        else ifNameSetFunc("ASECH", getASecH)
        else ifNameSetFunc("ACSCH", getACscH)
        else ifNameSetFunc("ACOTH", getACotH)
        else throw Pothos::InvalidArgumentException("Invalid operation", funcName);
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto* input = this->input(0);
        auto* output = this->output(0);

        const T* buffIn = input->buffer();
        T* buffOut = output->buffer();

        _func(buffIn, buffOut, elems);

        input->consume(elems);
        output->produce(elems);
    }

private:
    Func _func;
};

//
// Registration
//

static Pothos::Block* makeTrigonometric(
    const Pothos::DType& dtype,
    const std::string& operation)
{
    #define ifTypeDeclareTrig(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Trigonometric<T>(operation, dtype.dimension());

    ifTypeDeclareTrig(float)
    ifTypeDeclareTrig(double)

    throw Pothos::InvalidArgumentException(
              "makeTrigonometric: unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerTrigonometric(
    "/comms/trigonometric",
    Pothos::Callable(&makeTrigonometric));
