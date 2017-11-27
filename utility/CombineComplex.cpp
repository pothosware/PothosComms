// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <iostream>

/***********************************************************************
 * |PothosDoc Combine Complex
 *
 * Combine streams of real and imaginary components into a complex stream.
 *
 * |category /Utility
 * |category /Convert
 *
 * |param dtype[Data Type] The data type of the real and imaginary parts.
 * |widget DTypeChooser(float=1,int=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/combine_complex(dtype)
 **********************************************************************/
template <typename Type>
class CombineComplex : public Pothos::Block
{
public:
    CombineComplex(const size_t dimension)
    {
        _rePort = this->setupInput("re", Pothos::DType(typeid(Type), dimension));
        _imPort = this->setupInput("im", Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(std::complex<Type>), dimension));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minAllElements;
        if (elems == 0) return;

        //buffers
        auto outPort = this->output(0);
        std::complex<Type> *out = outPort->buffer();
        const Type *re = _rePort->buffer();
        const Type *im = _imPort->buffer();

        //convert
        const size_t N = elems*outPort->dtype().dimension();
        for (size_t i = 0; i < N; i++)
        {
            out[i] = std::complex<Type>(re[i], im[i]);
        }

        //produce/consume
        outPort->produce(elems);
        _rePort->consume(elems);
        _imPort->consume(elems);
    }

private:
    Pothos::InputPort *_rePort;
    Pothos::InputPort *_imPort;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *combineComplexFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(type))) return new CombineComplex<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("combineComplexFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerCombineComplex(
    "/comms/combine_complex", &combineComplexFactory);
