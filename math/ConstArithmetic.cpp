// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>

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
    using ArithFcn = void(*)(const T*, const T&, T*, size_t);
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

        _func(buffIn, _constant, buffOut, elems*input->dtype().dimension());

        input->consume(elems);
        output->produce(elems);
    }

private:
    T _constant;
    size_t _position;
    ArithFcn _func;
};

//
// Arithmetic functions
//

template <typename T>
void XPlusK(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] + k;
    }
}

template <typename T>
void XSubK(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] - k;
    }
}

template <typename T>
void KSubX(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = k - in[i];
    }
}

template <typename T>
void XMultK(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] * k;
    }
}

template <typename T>
void XDivK(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] / k;
    }
}

template <typename T>
void KDivX(const T* in, const T& k, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = k / in[i];
    }
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
        ifTypeDeclareFactory__(type, "X+K", XPlusK<type>) \
        ifTypeDeclareFactory__(type, "X-K", XSubK<type>) \
        ifTypeDeclareFactory__(type, "K-X", KSubX<type>) \
        ifTypeDeclareFactory__(type, "X*K", XMultK<type>) \
        ifTypeDeclareFactory__(type, "X/K", XDivK<type>) \
        ifTypeDeclareFactory__(type, "K/X", KDivX<type>) 
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
