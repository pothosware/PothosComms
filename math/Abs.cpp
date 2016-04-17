// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max
#include <type_traits>
#include "FxptHelpers.hpp"

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
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |factory /comms/abs(dtype)
 **********************************************************************/
template <typename InType, typename OutType>
class Abs : public Pothos::Block
{
public:
    Abs(const size_t dimension)
    {
        this->setupInput(0, Pothos::DType(typeid(InType), dimension));
        this->setupOutput(0, Pothos::DType(typeid(OutType), dimension));
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
        const size_t N = elems*inPort->dtype().dimension();
        for (size_t i = 0; i < N; i++)
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
        if (dtype == Pothos::DType(typeid(intype))) return new Abs<intype, outtype>(dtype.dimension());
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
