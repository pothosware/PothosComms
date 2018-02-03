// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <cmath>
#include "FFTAux.h"

/***********************************************************************
 * |PothosDoc FFT
 *
 * Perform a Fast Fourier Transform on input port 0
 * and produce the FFT result to output port 0.
 *
 * |category /FFT
 * |keywords dft fft fast fourier transform
 *
 * |param dtype[Data Type] The data type of the input and output element stream.
 * |widget DTypeChooser(cfloat=1, cint=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param numBins[Num FFT Bins] The number of bins per fourier transform.
 * |default 1024
 * |option 512
 * |option 1024
 * |option 2048
 * |option 4096
 * |widget ComboBox(editable=true)
 *
 * |param inverse[Inverse FFT] The option to perform the inverse for forward FFT.
 * |option [Forward] false
 * |option [Inverse] true
 * |default false
 *
 * |factory /comms/fft(dtype, numBins, inverse)
 **********************************************************************/
template <typename Type>
class FFT : public Pothos::Block
{
public:
    FFT(const size_t numBins, const bool inverse):
        _numBins(numBins),
        _inverse(inverse),
        _fftAux(numBins, inverse)
    {
        this->setupInput(0, typeid(Type));
        this->setupOutput(0, typeid(Type));
        this->input(0)->setReserve(_numBins);
    }

    //! Custom output buffer manager with slabs large enough for the fft result
    Pothos::BufferManager::Sptr getOutputBufferManager(const std::string &, const std::string &)
    {
        Pothos::BufferManagerArgs args;
        args.bufferSize = _numBins*sizeof(Type);
        return Pothos::BufferManager::make("generic", args);
    }

    void work(void)
    {
        auto inPort = this->input(0);
        auto outPort = this->output(0);

        _fftAux.transform(
            inPort->buffer().template as<const Type*>(),
            outPort->buffer().template as<Type*>());

        inPort->consume(_numBins);
        outPort->produce(_numBins);
    }

private:
    const size_t _numBins;
    const bool _inverse;
    FFTAux<Type> _fftAux;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *FFTFactory(const Pothos::DType &dtype, const size_t numBins, const bool inverse)
{
    #define ifTypeDeclareFactory__(Type) \
        if (dtype == Pothos::DType(typeid(Type))) return new FFT<Type>(numBins, inverse);
    #define ifTypeDeclareFactory(Type) \
        ifTypeDeclareFactory__(std::complex<Type>)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(kiss_fft_scalar);
    throw Pothos::InvalidArgumentException("FFTFactory("+dtype.toString()+")", "unsupported type");
}
static Pothos::BlockRegistry registerFFT(
    "/comms/fft", &FFTFactory);
