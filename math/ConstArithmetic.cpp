// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/ConstArithmetic_SIMDDispatcher.hpp"
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
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T>
using ConstArithmeticFcn = void(*)(const T*, const T&, T*, size_t);

#ifdef POTHOS_XSIMD

template <typename T>
using EnableForSIMDFcn = typename std::enable_if<!IsComplex<T>::value, ConstArithmeticFcn<T>>::type;

template <typename T>
using EnableForDefaultFcn = typename std::enable_if<IsComplex<T>::value, ConstArithmeticFcn<T>>::type;

#else

template <typename T>
using EnableForDefaultFcn = ConstArithmeticFcn<T>;

#endif

/***********************************************************************
 * |PothosDoc Const Arithmetic
 *
 * Perform arithmetic operations on each element, using a user-given
 * constant as an operand.
 *
 * |category /Math
 * |keywords math arithmetic add subtract multiply divide
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(int=1,uint1=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |param operation The mathematical operation to perform.
 * |widget ComboBox(editable=false)
 * |default "X+K"
 * |option [X + K] "X+K"
 * |option [X - K] "X-K"
 * |option [K - X] "K-X"
 * |option [X * K] "X*K"
 * |option [X / K] "X/K"
 * |option [K / X] "K/X"
 * |preview enable
 *
 * |param constant[Constant] The constant value to use in the operation.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |factory /comms/const_arithmetic(dtype,operation,constant)
 * |setter setConstant(constant)
 **********************************************************************/
template <typename T>
class ConstArithmetic: public Pothos::Block
{
public:
    using ArithFcn = ConstArithmeticFcn<T>;
    using Class = ConstArithmetic<T>;

    ConstArithmetic(
        ArithFcn func,
        const T& constant,
        size_t dimension
    ): Pothos::Block(),
       _constant(0),
       _func(func)
    {
        const Pothos::DType dtype(typeid(T), dimension);
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, constant));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));

        this->registerProbe("constant");
        this->registerSignal("constantChanged");

        // Call setter to emit signal
        this->setConstant(constant);
    }

    T constant() const
    {
        return _constant;
    }

    void setConstant(const T& constant)
    {
        _constant = constant;
        this->emitSignal("constantChanged", constant);
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

        _func(buffIn, _constant, buffOut, elems);

        input->consume(elems);
        output->produce(elems);
    }

private:
    T _constant;
    size_t _position;
    ArithFcn _func;
};

//
// Arithmetic function getters, called on class construction
//

#ifdef POTHOS_XSIMD

template <typename T>
static inline EnableForSIMDFcn<T> getXPlusKFcn()
{
    return PothosCommsSIMD::XPlusKDispatch<T>();
}

// TODO: remove when others exposed

template <typename T>
static inline EnableForSIMDFcn<T> getXSubKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] - k;
    };

    return impl;
}

template <typename T>
static inline EnableForSIMDFcn<T> getKSubXFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = k - in[i];
    };

    return impl;
}

template <typename T>
static inline EnableForSIMDFcn<T> getXMultKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] * k;
    };

    return impl;
}

template <typename T>
static inline EnableForSIMDFcn<T> getXDivKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] / k;
    };

    return impl;
}

template <typename T>
static inline EnableForSIMDFcn<T> getKDivXFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = k / in[i];
    };

    return impl;
}

#endif

template <typename T>
static inline EnableForDefaultFcn<T> getXPlusKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] + k;
    };

    return impl;
}

template <typename T>
static inline EnableForDefaultFcn<T> getXSubKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] - k;
    };

    return impl;
}

template <typename T>
static inline EnableForDefaultFcn<T> getKSubXFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = k - in[i];
    };

    return impl;
}

template <typename T>
static inline EnableForDefaultFcn<T> getXMultKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] * k;
    };

    return impl;
}

template <typename T>
static inline EnableForDefaultFcn<T> getXDivKFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = in[i] / k;
    };

    return impl;
}

template <typename T>
static inline EnableForDefaultFcn<T> getKDivXFcn()
{
    static const auto impl = [](const T* in, const T& k, T* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++) out[i] = k / in[i];
    };

    return impl;
}

//
// Registration
//

static Pothos::Block* makeConstArithmetic(
    const Pothos::DType& dtype,
    const std::string& operation,
    const Pothos::Object& constant)
{
    #define ifTypeDeclareFactory__(type, opKey, func) \
        if((Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) && (opKey == operation)) \
            return new ConstArithmetic<type>(func, constant.convert<type>(), dtype.dimension());
    #define ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory__(type, "X+K", getXPlusKFcn<type>()) \
        ifTypeDeclareFactory__(type, "X-K", getXSubKFcn<type>()) \
        ifTypeDeclareFactory__(type, "K-X", getKSubXFcn<type>()) \
        ifTypeDeclareFactory__(type, "X*K", getXMultKFcn<type>()) \
        ifTypeDeclareFactory__(type, "X/K", getXDivKFcn<type>()) \
        ifTypeDeclareFactory__(type, "K/X", getKDivXFcn<type>()) 
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory_(std::complex<type>)

    ifTypeDeclareFactory(std::int8_t)
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "makeConstArithmetic("+dtype.toString()+", operation="+operation+")",
              "unsupported args");
}

static Pothos::BlockRegistry registerConstArithmetic(
    "/comms/const_arithmetic",
    Pothos::Callable(&makeConstArithmetic));
