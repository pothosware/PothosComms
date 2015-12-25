// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max

/***********************************************************************
 * |PothosDoc Conjugate
 *
 * Take the complex conjugate of every input element.
 *
 * out[n] = conj(in[n])
 *
 * |category /Math
 * |keywords math conjugate complex conj
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(cfloat=1,cint=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |factory /comms/conjugate(dtype)
 **********************************************************************/
template <typename Type>
class Conjugate : public Pothos::Block
{
public:
    Conjugate(void)
    {
        this->setupInput(0, typeid(Type));
        this->setupOutput(0, typeid(Type));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //get pointers to in and out buffer
        auto inPort = this->input(0);
        auto outPort = this->output(0);
        auto in = inPort->buffer().template as<const Type *>();
        auto out = outPort->buffer().template as<Type *>();

        //perform conjugate operation
        for (size_t i = 0; i < elems; i++)
        {
            out[i] = std::conj(in[i]);
        }

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *conjugateFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(type) \
        if (dtype == Pothos::DType(typeid(type))) return new Conjugate<type>();
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(std::complex<type>)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("conjugateFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerConjugate(
    "/comms/conjugate", &conjugateFactory);
