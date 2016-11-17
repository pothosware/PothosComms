// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <iostream>
#include <algorithm> //min/max
#include <chrono>

/***********************************************************************
 * |PothosDoc Signal Probe
 *
 * The signal probe block records the last calculation from a stream of elements.
 * The signal probe has a slot called "probeValue" will will cause
 * a signal named "valueTriggered" to emit the most recent value.
 * The probe will also emit the value automatically at the specified rate
 * using the "valueChanged" signal.
 *
 * The calculation for value can be, the last seen value,
 * the RMS (root mean square) over the last buffer,
 * or the mean (average value) over the last buffer.
 *
 * |category /Utility
 * |category /Event
 * |keywords rms average mean
 * |alias /blocks/stream_probe
 *
 * |param dtype[Data Type] The data type consumed by the stream probe.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param mode The calculation mode for the value.
 * In value mode, this block expects to be fed by an upstream block
 * that produces a stream of slow-changing values.
 * Otherwise the value will appear random.
 * |default "VALUE"
 * |option [Value] "VALUE"
 * |option [RMS] "RMS"
 * |option [Mean] "MEAN"
 *
 * |param rate How many calculations per second?
 * The probe will perform a calculation at most this many times per second.
 * Incoming samples will be dropped and not processed between calculations.
 * A special value of 0.0 means perform the calculation on every input window.
 * |preview valid
 * |default 0.0
 *
 * |param window How many elements to calculate over?
 * |default 1024
 *
 * |factory /comms/signal_probe(dtype)
 * |setter setMode(mode)
 * |setter setRate(rate)
 * |setter setWindow(window)
 **********************************************************************/
template <typename Type, typename ProbeType>
class SignalProbe : public Pothos::Block
{
public:
    SignalProbe(void):
        _value(0),
        _mode("VALUE"),
        _window(1024),
        _rate(0.0)
    {
        this->setupInput(0, typeid(Type));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, value));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, setMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, getMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, setWindow));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, getWindow));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, setRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(SignalProbe, getRate));
        this->registerProbe("value");
        this->registerSignal("valueChanged");
        this->input(0)->setReserve(1);
    }

    ProbeType value(void)
    {
        return _value;
    }

    void setMode(const std::string &mode)
    {
        _mode = mode;
    }

    std::string getMode(void) const
    {
        return _mode;
    }

    void setWindow(const size_t window)
    {
        _window = window;
        this->input(0)->setReserve(window);
    }

    size_t getWindow(void) const
    {
        return _window;
    }

    void setRate(const double rate)
    {
        _rate = rate;
    }

    double getRate(void) const
    {
        return _rate;
    }

    void activate(void)
    {
        _nextCalc = std::chrono::high_resolution_clock::now();
    }

    void work(void)
    {
        auto inPort = this->input(0);
        auto x = inPort->buffer().template as<const Type *>();
        const auto N = std::min(_window, inPort->elements());
        inPort->consume(N);

        //check if the time expired or rate is 0.0
        auto currentTime = std::chrono::high_resolution_clock::now();
        if (_rate != 0.0 and currentTime < _nextCalc) return;

        //increment for the next calculation time
        if (_rate != 0.0)
        {
            const auto tps = std::chrono::nanoseconds((long long)(1e9/_rate));
            _nextCalc += std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(tps);
        }

        if (_mode == "VALUE") _value = x[N-1];
        else if (_mode == "RMS")
        {
            double accumulator = 0.0;
            ProbeType x_n;
            for (size_t n = 0; n < N; n++)
            {
                x_n = x[n];
                const double v = std::abs(x_n);
                accumulator += v*v;
            }
            _value = std::sqrt(accumulator/N);
        }
        else if (_mode == "MEAN")
        {
            ProbeType mean = 0;
            for (size_t n = 0; n < N; n++) mean += x[n];
            mean /= N;
            _value = mean;
        }

        this->emitSignal("valueChanged", _value);
    }

private:
    ProbeType _value;
    std::string _mode;
    size_t _window;
    double _rate;
    std::chrono::high_resolution_clock::time_point _nextCalc;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *signalProbeFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(type))) return new SignalProbe<type, double>(); \
        if (dtype == Pothos::DType(typeid(std::complex<type>))) return new SignalProbe<std::complex<type>, std::complex<double>>();
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("signalProbeFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerSignalProbe(
    "/comms/signal_probe", &signalProbeFactory);

static Pothos::BlockRegistry registerSignalProbeOldPath(
    "/blocks/stream_probe", &signalProbeFactory);
