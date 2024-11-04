// Copyright (c) 2014-2018 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/Logger.h>
#include <cmath>
#include <complex>
#include <algorithm>
#include <iostream>
#include <spuce/filters/design_window.h>

using spuce::design_window;
/***********************************************************************
 * |PothosDoc Window Designer
 *
 * Designer for window taps.
 * This block emits a "tapsChanged" signal upon activations,
 * and when one of the parameters is modified.
 * The "tapsChanged" signal contains an array of window taps,
 * and can be connected to a vector source's set elements method.
 *
 * |category /Window
 * |keywords window rect rectangular hann hamming blackman bartlett flattop kaiser chebyshev
 *
 * |param window[Window Type] The window function controls the window shape.
 * |default "hann"
 * |option [Rectangular] "rectangular"
 * |option [Hann] "hann"
 * |option [Hamming] "hamming"
 * |option [Blackman] "blackman"
 * |option [Bartlett] "bartlett"
 * |option [Flat-top] "flattop"
 * |option [Kaiser] "kaiser"
 * |option [Chebyshev] "chebyshev"
 *
 * |param windowArgs[Window Args] Optional window arguments (depends on window type).
 * <ul>
 * <li>When using the <i>Kaiser</i> window, specify [beta] to use the parameterized Kaiser window.</li>
 * <li>When using the <i>Chebyshev</i> window, specify [atten] to use the Dolph-Chebyshev window with attenuation in dB.</li>
 * </ul>
 * |default []
 * |preview valid
 *
 * |param numTaps[Num Taps] The number of window taps.
 * |default 51
 * |widget SpinBox(minimum=1)
 *
 * |param normalization[Normalization] The option to normalize the output taps.
 * When normalized, the output taps will be the window taps divided by the
 * selected metric computed over the window taps.
 * |option [None] "NONE"
 * |option [Sum] "SUM"
 * |option [Power] "POWER"
 * |default "NONE"
 *
 * |factory /comms/window_designer()
 * |setter setWindowType(window)
 * |setter setWindowArgs(windowArgs)
 * |setter setNumTaps(numTaps)
 * |setter setNormalization(normalization)
 **********************************************************************/
class WindowDesigner : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new WindowDesigner();
    }

    WindowDesigner(void):
        _windowType("hann"),
        _numTaps(51),
        _normalization("NONE")
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, setWindowType));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, windowType));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, setWindowArgs));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, windowArgs));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, setNumTaps));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, numTaps));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, setNormalization));
        this->registerCall(this, POTHOS_FCN_TUPLE(WindowDesigner, normalization));
        this->registerSignal("tapsChanged");
        this->recalculate();
    }

    void setWindowType(const std::string &type)
    {
        _windowType = type;
        this->recalculate();
    }

    std::string windowType(void) const
    {
        return _windowType;
    }

    void setWindowArgs(const std::vector<double> &args)
    {
        _windowArgs = args;
        this->recalculate();
    }

    std::vector<double> windowArgs(void) const
    {
        return _windowArgs;
    }

    void setNumTaps(const size_t num)
    {
        _numTaps = num;
        this->recalculate();
    }

    size_t numTaps(void) const
    {
        return _numTaps;
    }

    void setNormalization(const std::string &normalization)
    {
        _normalization = normalization;
        this->recalculate();
    }

    std::string normalization(void) const
    {
        return _normalization;
    }

    void activate(void)
    {
        this->recalculate();
    }

private:

    void recalculate(void);

    std::string _windowType;
    std::vector<double> _windowArgs;
    size_t _numTaps;
    std::string _normalization;
};

void WindowDesigner::recalculate(void)
{
    if (not this->isActive()) return;
    
    //check for error
    if (_numTaps == 0) throw Pothos::Exception("WindowDesigner()", "num taps must be positive");

    //generate the window
    auto window = design_window(_windowType, _numTaps, _windowArgs.empty()?0.0:_windowArgs.at(0));

    if (_normalization == "SUM")
    {
        double accumulator = 0.0;
        for (size_t n = 0; n < _numTaps; n++)
        {
            accumulator += window[n];
        }
        for (size_t n = 0; n < _numTaps; n++)
        {
            window[n] /= accumulator;
        }
    }
    else if (_normalization == "POWER")
    {
        double power = 0.0;
        for (size_t n = 0; n < _numTaps; n++)
        {
            power += window[n]*window[n];
        }
        power = std::sqrt(power/_numTaps);
        for (size_t n = 0; n < _numTaps; n++)
        {
            window[n] /= power;
        }
    }

    this->emitSignal("tapsChanged", window);
}

static Pothos::BlockRegistry registerWindowDesigner(
    "/comms/window_designer", &WindowDesigner::make);
