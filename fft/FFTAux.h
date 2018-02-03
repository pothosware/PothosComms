#pragma once

#include <complex>

#include "kissfft.hh"
#include "kiss_fft.h"

template<typename Type>
class FFTAux {
private:
    // Don't allow to use this class without specialization.
    FFTAux() = delete;
    FFTAux(size_t numBins, bool inverse) = delete;
};

template<typename Type>
class FFTAux<std::complex<Type>> {
public:
    inline FFTAux(size_t numBins, bool inverse) : _fftFloat(numBins, inverse) {}

    inline void transform(const std::complex<Type> *input, std::complex<Type> *output) {
        _fftFloat.transform(input, output);
    }

private:
    kissfft<Type> _fftFloat;
};

template<>
class FFTAux<std::complex<kiss_fft_scalar>> {
public:
    inline FFTAux(size_t numBins, bool inverse) : _fftFixed(nullptr) {
        _fftFixed = kiss_fft_alloc(numBins, inverse, nullptr, nullptr);
    }

    inline ~FFTAux() {
        kiss_fft_free(_fftFixed);
    }

    inline void transform(const std::complex<kiss_fft_scalar> *input, std::complex<kiss_fft_scalar> *output) {
        kiss_fft(_fftFixed,
            reinterpret_cast<const kiss_fft_cpx*>(input),
            reinterpret_cast<kiss_fft_cpx*>(output));
    }

private:
    kiss_fft_cfg _fftFixed;
};
