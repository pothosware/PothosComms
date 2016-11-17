// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <iostream>

/***********************************************************************
 * |PothosDoc Split Complex
 *
 * Split a stream of complex numbers into the real and imaginary components.
 *
 * |category /Utility
 * |category /Convert
 *
 * |param dtype[Data Type] The data type of the real and imaginary parts.
 * |widget DTypeChooser(float=1,int=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/split_complex(dtype)
 **********************************************************************/
template <typename Type>
class SplitComplex : public Pothos::Block
{
public:
    SplitComplex(const size_t dimension)
    {
        this->setupInput(0, Pothos::DType(typeid(std::complex<Type>), dimension));
        _rePort = this->setupOutput("re", Pothos::DType(typeid(Type), dimension));
        _imPort = this->setupOutput("im", Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;
    }

private:
    Pothos::OutputPort *_rePort;
    Pothos::OutputPort *_imPort;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *splitComplexFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(type))) return new SplitComplex<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("splitComplexFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerSplitComplex(
    "/comms/split_complex", &splitComplexFactory);
