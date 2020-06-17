// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>

template <typename T>
static void arrayCos(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::cos(in[i]);
    }
}

template <typename T>
static void arraySin(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::sin(in[i]);
    }
}

template <typename T>
static void arrayTan(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::tan(in[i]);
    }
}

template <typename T>
static void arraySec(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::cos(in[i]);
    }
}

template <typename T>
static void arrayCsc(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::sin(in[i]);
    }
}

template <typename T>
static void arrayCot(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::tan(in[i]);
    }
}

template <typename T>
static void arrayACos(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::acos(in[i]);
    }
}

template <typename T>
static void arrayASin(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::asin(in[i]);
    }
}

template <typename T>
static void arrayATan(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::atan(in[i]);
    }
}

template <typename T>
static void arrayASec(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::acos(T(1.0) / in[i]);
    }
}

template <typename T>
static void arrayACsc(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::asin(T(1.0) / in[i]);
    }
}

template <typename T>
static void arrayACot(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::atan(T(1.0) / in[i]);
    }
}

template <typename T>
static void arrayCosH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::cosh(in[i]);
    }
}

template <typename T>
static void arraySinH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::sinh(in[i]);
    }
}

template <typename T>
static void arrayTanH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::tanh(in[i]);
    }
}

template <typename T>
static void arraySecH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::cosh(in[i]);
    }
}

template <typename T>
static void arrayCscH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::sinh(in[i]);
    }
}

template <typename T>
static void arrayCotH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = T(1.0) / std::tanh(in[i]);
    }
}

template <typename T>
static void arrayACosH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::acosh(in[i]);
    }
}

template <typename T>
static void arrayASinH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::asinh(in[i]);
    }
}

template <typename T>
static void arrayATanH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::atanh(in[i]);
    }
}

template <typename T>
static void arrayASecH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::acosh(T(1.0) / in[i]);
    }
}

template <typename T>
static void arrayACscH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::asinh(T(1.0) / in[i]);
    }
}

template <typename T>
static void arrayACotH(const T* in, T* out, size_t num)
{
    for(size_t i = 0; i < num; ++i)
    {
        out[i] = std::atanh(T(1.0) / in[i]);
    }
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
    using Func = T(*)(const T*, T*, size_t);
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
        #define ifNameSetFunc(name,func) \
            if(name == funcName) _func = Func(func<T>);

        ifNameSetFunc(     "COS",   arrayCos)
        else ifNameSetFunc("SIN",   arraySin)
        else ifNameSetFunc("TAN",   arrayTan)
        else ifNameSetFunc("SEC",   arraySec)
        else ifNameSetFunc("CSC",   arrayCsc)
        else ifNameSetFunc("COT",   arrayCot)
        else ifNameSetFunc("ACOS",  arrayACos)
        else ifNameSetFunc("ASIN",  arrayASin)
        else ifNameSetFunc("ATAN",  arrayATan)
        else ifNameSetFunc("ASEC",  arrayASec)
        else ifNameSetFunc("ACSC",  arrayACsc)
        else ifNameSetFunc("ACOT",  arrayACot)
        else ifNameSetFunc("COSH",  arrayCosH)
        else ifNameSetFunc("SINH",  arraySinH)
        else ifNameSetFunc("TANH",  arrayTanH)
        else ifNameSetFunc("SECH",  arraySecH)
        else ifNameSetFunc("CSCH",  arrayCscH)
        else ifNameSetFunc("COTH",  arrayCotH)
        else ifNameSetFunc("ACOSH", arrayACosH)
        else ifNameSetFunc("ASINH", arrayASinH)
        else ifNameSetFunc("ATANH", arrayATanH)
        else ifNameSetFunc("ASECH", arrayASecH)
        else ifNameSetFunc("ACSCH", arrayACscH)
        else ifNameSetFunc("ACOTH", arrayACotH)
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
