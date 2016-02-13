// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <iostream>
#include <algorithm> //min/max
#include "FxptHelpers.hpp"

/***********************************************************************
 * |PothosDoc Freq Demod
 *
 * The frequency demodulation block consumes a complex input stream
 * on input port 0, performs a differential atan2 operation,
 * and outputs the real-valued changes in frequency
 * to the output stream on output port 0.
 *
 * |category /Demod
 * |keywords frequency modulation fm atan differential
 *
 * |param dtype[Data Type] The input data type.
 * The output type is always real.
 * The floating point outputs are in radians between -pi and +pi.
 * The fixed point outputs use a signed 16-bit range to represent -pi
 * through +pi (non-inclusive).
 * |widget DTypeChooser(cfloat=1,cint=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |factory /comms/freq_demod(dtype)
 **********************************************************************/
template <typename InType, typename OutType>
class FreqDemod : public Pothos::Block
{
public:

    FreqDemod(void)
    {
        this->setupInput(0, typeid(InType));
        this->setupOutput(0, typeid(OutType));
    }

    void activate(void)
    {
        _prev = 0;
    }

    void work(void)
    {
        auto inPort = this->input(0);
        auto outPort = this->output(0);

        const size_t N = this->workInfo().minElements;

        //cast the input and output buffers
        const auto in = inPort->buffer().template as<const InType *>();
        const auto out = outPort->buffer().template as<OutType *>();

        for (size_t i = 0; i < N; i++)
        {
            auto in_i = in[i];
            auto diff = in_i * _prev;
            auto angle = getAngle(diff);
            _prev = std::conj(in_i);
            out[i] = OutType(angle);
        }

        inPort->consume(N);
        outPort->produce(N);
    }

private:
    InType _prev;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *FreqDemodFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(intype, outtype) \
        if (dtype == Pothos::DType(typeid(intype))) return new FreqDemod<intype, outtype>();
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(std::complex<type>, type)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("FreqDemodFactory("+dtype.toString()+")", "unsupported types");
}
static Pothos::BlockRegistry registerFreqDemod(
    "/comms/freq_demod", &FreqDemodFactory);
