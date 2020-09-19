// Copyright (c) 2015-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max
#include <type_traits>
#include "FxptHelpers.hpp"

//
// Implementation getters to be called on class construction
//

template <typename InType, typename OutType>
using AbsFcn = void(*)(const InType*, OutType*, const size_t);

#ifdef POTHOS_XSIMD

template <typename InType, typename OutType>
static typename std::enable_if<std::is_same<InType, OutType>::value, AbsFcn<InType, OutType>>::type getAbsFcn()
{
    return PothosCommsSIMD::absDispatch<InType>();
}

template <typename InType, typename OutType>
static typename std::enable_if<!std::is_same<InType, OutType>::value, AbsFcn<InType, OutType>>::type
#else
template <typename InType, typename OutType>
static AbsFcn<InType, OutType>
#endif
getAbsFcn()
{
    return [](const InType* in, OutType* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = getAbs<OutType>(in[i]);
    };
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
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,dim=1)
 * |default "complex_float32"
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
        const InType *in = inPort->buffer();
        OutType *out = outPort->buffer();

        //perform abs operation
        const size_t N = elems*inPort->dtype().dimension();
        _absFcn(in, out, N);

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    static AbsFcn<InType, OutType> _absFcn;
};

template <typename InType, typename OutType>
AbsFcn<InType, OutType> Abs<InType, OutType>::_absFcn = getAbsFcn<InType, OutType>();

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *absFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(intype, outtype) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(intype))) \
            return new Abs<intype, outtype>(dtype.dimension());
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
