// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/Format.h>

#include <algorithm>
#include <complex>
#include <cmath>
#include <string>

template <typename T>
static T waveTodB(const std::complex<T>& wave)
{
    return T(20.0) * std::log10(std::abs(wave));
}

template <typename T>
static T waveTodBm(const std::complex<T>& wave)
{
    return T(10.0) * std::log10(std::abs(wave));
}

/***********************************************************************
 * |PothosDoc Wave Conversion
 *
 * Convert complex waves to scalar power or field quantity units on a logarithmic scale.
 *
 * |category /Math
 * |keywords math db dbm decibel log power
 *
 * |param dtype[Data Type] The floating-point data type. Input will be complex of this type.
 * |widget DTypeChooser(float=1)
 * |default "float32"
 * |preview disable
 *
 * |param unit[Unit] The output type for the incoming waves.
 *
 * <ul>
 * <li><b>dB:</b> expresses the ratio from one power to another, with no specific reference</li>
 * <li><b>dBm:</b> expresses the ratio of the power to a 1 mW reference point.</li>
 * </ul>
 *
 * |widget ComboBox(editable=false)
 * |option [dB] "dB"
 * |option [dBm] "dBm"
 *
 * |factory /comms/wave_conversion(dtype)
 * |setter setUnit(unit)
 **********************************************************************/
template <typename T>
class WaveConversion: public Pothos::Block
{
public:
    using Class = WaveConversion<T>;
    using WaveConversionFcn = T(*)(const std::complex<T>&);

    WaveConversion(size_t dimension):
        _unit(),
        _func(nullptr)
    {
        this->setupInput(0, Pothos::DType(typeid(std::complex<T>), dimension));
        this->setupOutput(0, Pothos::DType(typeid(T), dimension));

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, getUnit));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setUnit));

        this->setUnit("dB");
    }

    std::string getUnit() const
    {
        return _unit;
    }

    void setUnit(const std::string& unit)
    {
        if("dB" == unit)
        {
            _func = &waveTodB<T>;
        }
        else if("dBm" == unit)
        {
            _func = &waveTodBm<T>;
        }
        else
        {
            throw Pothos::InvalidArgumentException(
                      "Invalid unit",
                      unit);
        }

        _unit = unit;
    }

    void work() override
    {
        if(nullptr == _func)
        {
            throw Pothos::AssertionViolationException(
                      "Internal function pointer is null",
                      Poco::format("Unit = %s", _unit));
        }

        auto inputPort = this->input(0);
        auto outputPort = this->output(0);

        const auto numElements = std::min(inputPort->elements(), outputPort->elements());
        if(0 == numElements)
        {
            return;
        }

        const std::complex<T>* input = inputPort->buffer();
        T* output = outputPort->buffer();
        for(size_t i = 0; i < numElements; ++i)
        {
            output[i] = _func(input[i]);
        }

        inputPort->consume(numElements);
        outputPort->produce(numElements);
    }

private:
    std::string _unit;
    WaveConversionFcn _func;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block* makeWaveConversion(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new WaveConversion<type>(dtype.dimension());

    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(double);

    throw Pothos::InvalidArgumentException(
              "Invalid or unsupported type",
              dtype.name());
}

static Pothos::BlockRegistry registerWaveConversion(
    "/comms/wave_conversion", &makeWaveConversion);
