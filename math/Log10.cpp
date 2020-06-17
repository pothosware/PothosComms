// Copyright (c) 2019 Nick Foster
//               2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <algorithm> //min/max
#include <cmath>

template <typename Type>
static void arrayLog10(const Type *in, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        out[i] = std::log10(in[i]);
    }
}

/***********************************************************************
 * |PothosDoc Log10
 *
 * Perform the base 10 logarithm on every input element.
 *
 * out[n] = log10(in[n])
 *
 * |category /Math
 * |keywords math log log10
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/log10(dtype)
 **********************************************************************/
template <typename Type>
class Log10 : public Pothos::Block
{
public:
    Log10(const size_t dimension)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //get pointers to in and out buffer
        auto inPort = this->input(0);
        auto outPort = this->output(0);
        const Type *in = inPort->buffer();
        Type *out = outPort->buffer();

        //do the needful
        const size_t N = elems*inPort->dtype().dimension();
        arrayLog10(in, out, N);

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *log10Factory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Log10<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("log10Factory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerlog10(
    "/comms/log10", &log10Factory);
