// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>
#include <vector>

//
// Arithmetic functions
//

template <typename T>
void XPlusK(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] + kVec[i % kVec.size()];
    }
}

template <typename T>
void XSubK(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] - kVec[i % kVec.size()];
    }
}

template <typename T>
void KSubX(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = kVec[i % kVec.size()] - in[i];
    }
}

template <typename T>
void XMultK(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] * kVec[i % kVec.size()];
    }
}

template <typename T>
void XDivK(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = in[i] / kVec[i % kVec.size()];
    }
}

template <typename T>
void KDivX(const T* in, const std::vector<T>& kVec, T* out, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        out[i] = kVec[i % kVec.size()] / in[i];
    }
}

//
// Test class
//

template <typename T, void (*Operator)(const T*, const std::vector<T>&, T*, size_t)>
class VectorArithmetic: public Pothos::Block
{
public:
    using Class = VectorArithmetic<T, Operator>;

    VectorArithmetic(size_t dimension):
       Pothos::Block(),
       _vector({0})
    {
        const Pothos::DType dtype(typeid(T), dimension);
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, vector));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setVector));

        this->registerProbe("vector");
        this->registerSignal("vectorChanged");

        // Call setter to emit signal
        this->setVector(_vector);
    }

    std::vector<T> vector() const
    {
        return _vector;
    }

    void setVector(const std::vector<T>& vector)
    {
        if(vector.empty())
        {
            throw Pothos::InvalidArgumentException("Given vector cannot be empty.");
        }

        _vector = vector;
        this->emitSignal("vectorChanged", vector);
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

        Operator(buffIn, _vector, buffOut, elems);

        input->consume(elems);
        output->produce(elems);
    }

private:
    std::vector<T> _vector;
};

//
// Registration
//

static Pothos::Block* makeVectorArithmetic(
    const Pothos::DType& dtype,
    const std::string& operation)
{
    #define ifTypeDeclareFactory__(type, opKey, func) \
        if((Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) && (opKey == operation)) \
            return new VectorArithmetic<type, func<type>>(dtype.dimension());
    #define ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory__(type, "X+K", XPlusK) \
        ifTypeDeclareFactory__(type, "X-K", XSubK) \
        ifTypeDeclareFactory__(type, "K-X", KSubX) \
        ifTypeDeclareFactory__(type, "X*K", XMultK) \
        ifTypeDeclareFactory__(type, "X/K", XDivK) \
        ifTypeDeclareFactory__(type, "K/X", KDivX) 
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
              "makeVectorArithmetic("+dtype.toString()+", operation="+operation+")",
              "unsupported args");
}

/***********************************************************************
 * |PothosDoc Vector Arithmetic
 *
 * Perform arithmetic operations on each element, using a user-given vector
 * of values, such that each element corresponds to the element at the given
 * position in the vector, modulus the vector size.
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
 * |param vector[Vector] The constant value to use in the operation.
 * |widget LineEdit()
 * |default [0]
 * |preview enable
 *
 * |factory /comms/vector_arithmetic(dtype,operation)
 * |setter setVector(vector)
 **********************************************************************/
static Pothos::BlockRegistry registerVectorArithmetic(
    "/comms/vector_arithmetic",
    Pothos::Callable(&makeVectorArithmetic));
