// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>

//
// Reciprocal functions
//

template <typename T>
static T sec(T x)
{
    return T(1.0) / std::cos(x);
}

template <typename T>
static T csc(T x)
{
    return T(1.0) / std::sin(x);
}

template <typename T>
static T cot(T x)
{
    return T(1.0) / std::tan(x);
}

template <typename T>
static T asec(T x)
{
    return std::acos(T(1.0) / x);
}

template <typename T>
static T acsc(T x)
{
    return std::asin(T(1.0) / x);
}

template <typename T>
static T acot(T x)
{
    return std::atan(T(1.0) / x);
}

template <typename T>
static T sech(T x)
{
    return T(1.0) / std::cosh(x);
}

template <typename T>
static T csch(T x)
{
    return T(1.0) / std::sinh(x);
}

template <typename T>
static T coth(T x)
{
    return T(1.0) / std::tanh(x);
}

template <typename T>
static T asech(T x)
{
    return std::acosh(T(1.0) / x);
}

template <typename T>
static T acsch(T x)
{
    return std::asinh(T(1.0) / x);
}

template <typename T>
static T acoth(T x)
{
    return std::atanh(T(1.0) / x);
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
    using Func = T(*)(T);
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
            if(name == funcName) _func = Func(func);

        ifNameSetFunc(     "COS",   std::cos)
        else ifNameSetFunc("SIN",   std::sin)
        else ifNameSetFunc("TAN",   std::tan)
        else ifNameSetFunc("SEC",   sec)
        else ifNameSetFunc("CSC",   csc)
        else ifNameSetFunc("COT",   cot)
        else ifNameSetFunc("ACOS",  std::acos)
        else ifNameSetFunc("ASIN",  std::asin)
        else ifNameSetFunc("ATAN",  std::atan)
        else ifNameSetFunc("ASEC",  asec)
        else ifNameSetFunc("ACSC",  acsc)
        else ifNameSetFunc("ACOT",  acot)
        else ifNameSetFunc("COSH",  std::cosh)
        else ifNameSetFunc("SINH",  std::sinh)
        else ifNameSetFunc("TANH",  std::tanh)
        else ifNameSetFunc("SECH",  sech)
        else ifNameSetFunc("CSCH",  csch)
        else ifNameSetFunc("COTH",  coth)
        else ifNameSetFunc("ACOSH", std::acosh)
        else ifNameSetFunc("ASINH", std::asinh)
        else ifNameSetFunc("ATANH", std::atanh)
        else ifNameSetFunc("ASECH", asech)
        else ifNameSetFunc("ACSCH", acsch)
        else ifNameSetFunc("ACOTH", acoth)
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

        for(size_t elem = 0; elem < elems; ++elem)
        {
            buffOut[elem] = _func(buffIn[elem]);
        }

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
