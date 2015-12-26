// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max
#include <type_traits>

//absolute value for floating point and real integer types
template <typename OutType, typename InType>
OutType getAbs(const InType &in)
{
    return OutType(std::abs(in));
}

//absolute value for fixed point complex types
template <typename OutType, typename InType>
typename std::enable_if<std::is_integral<OutType>::value, OutType>::type
getAbs(const std::complex<InType> &in)
{
    const auto mag2 = in.real()*in.real() + in.imag()*in.imag();
    return OutType(std::sqrt(float(mag2)));
}

/***********************************************************************
 * |PothosDoc Abs
 *
 * Perform abs() on every input element.
 * For real inputs, this produces the absolute value.
 * For complex inputs, this produces the magnitude.
 *
 * out[n] = abs(in[n])
 *
 * |category /Math
 * |keywords math abs magnitude absolute
 *
 * |param dtype[Data Type] The input data type.
 * The output type is always real.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |factory /comms/abs(dtype)
 **********************************************************************/
template <typename InType, typename OutType>
class Abs : public Pothos::Block
{
public:
    Abs(void)
    {
        this->setupInput(0, typeid(InType));
        this->setupOutput(0, typeid(OutType));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //get pointers to in and out buffer
        auto inPort = this->input(0);
        auto outPort = this->output(0);
        auto in = inPort->buffer().template as<const InType *>();
        auto out = outPort->buffer().template as<OutType *>();

        //perform abs operation
        for (size_t i = 0; i < elems; i++)
        {
            out[i] = getAbs<OutType>(in[i]);
        }

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *absFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(intype, outtype) \
        if (dtype == Pothos::DType(typeid(intype))) return new Abs<intype, outtype>();
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(type, type) \
        ifTypeDeclareFactory_(std::complex<type>, type)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("absFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerAbs(
    "/comms/abs", &absFactory);
