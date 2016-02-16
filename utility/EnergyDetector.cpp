// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cmath>
#include <cstdint>
#include <complex>
#include <iostream>
#include <algorithm> //min/max

/***********************************************************************
 * |PothosDoc Energy Detector
 *
 * The energy detector inspects a stream for regions of increased energy.
 * The detector can be used to discern packet bursts amongst the background noise
 * for the purposes of plotting or removing unusable samples from a stream.
 * Regions of high energy are detected based on programmable thresholds,
 * and can be forwarded selectively to the output or decorated with labels.
 *
 * |category /Utility
 * |category /Packet
 * |keywords burst packet evergy detector trigger
 *
 * |param dtype[Data Type] The data type processed by the detector.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param forwardMode[Forwarding Mode] Forward all or just active samples.
 * |option [All samples] "ALL"
 * |option [Active only] "ACTIVE"
 * |default "ACTIVE"
 *
 * |param signalAverage[Signal Average] The signal averager decay time constant in samples.
 * This parameter effectively controls the averaging detector for energy present.
 * Single pole filter roll-off constant: gainSignalAverage = exp(-1/signalAverage)
 * |default 10
 * |units samples
 *
 * |param noiseAverage[Noise Average] The noise averager decay time constant in samples.
 * The detector uses this filter to determine the noise floor when inactive.
 * Single pole filter roll-off constant: gainNoiseAverage = exp(-1/noiseAverage)
 * |default 100
 * |units samples
 *
 * |param lookahead A configurable input delay to compensate for envelope lag.
 * Without lookahead, the envelope calculation lags behind the input due to filtering.
 * The lookahead compensation adjusts the envelope to match up with the input events.
 * |default 10
 * |units samples
 *
 * |param activationLevel[Activation Level] The threshold level that the input must exceed to activate.
 * This threshold level is a power in dB that is relative to the noise floor.
 * |default 3.0
 * |units dB
 * |tab Thresholds
 *
 * |param activationMin[Activation Minimum] The minimum number of samples to remain activated.
 * The detector will remain active for at least this many samples reguardless of the deactivation threshold.
 * Once the minimum number of samples has been reached, the deactivation threshold comes into play.
 * |default 0
 * |preview valid
 * |units samples
 * |tab Thresholds
 *
 * |param activationMax[Activation Maximum] The maximum number of samples to remain activated.
 * The detector will remain active for at most many samples reguardless of the deactivation threshold.
 * Once the maximum number of samples has been reached, the deactivation state is forced.
 * A maximum of 0 means that no maximum will be enforced.
 * |default 0
 * |preview valid
 * |units samples
 * |tab Thresholds
 *
 * |param deactivationLevel[Deactivation Level] The threshold level that the input must fall-below to deactivate.
 * This threshold level is a power in dB that is relative to the noise floor.
 * |default 3.0
 * |units dB
 * |tab Thresholds
 *
 * |param deactivationMin[Deactivation Minimum] The minimum number of samples to remain inactive.
 * The detector will remain inactive for at least this many samples reguardless of the activation threshold.
 * Once the minimum number of samples has been reached, the activation threshold comes into play.
 * |default 0
 * |preview valid
 * |units samples
 * |tab Thresholds
 *
 * |param deactivationMax[Deactivation Maximum] The maximum number of samples to remain inactive.
 * The detector will remain inactive for at most many samples reguardless of the activation threshold.
 * Once the maximum number of samples has been reached, the activation state is forced.
 * A maximum of 0 means that no maximum will be enforced.
 * |default 0
 * |preview valid
 * |units samples
 * |tab Thresholds
 *
 * |param activationId[Activation ID] The label ID to mark the element that crosses the activation threshold (when inactive).
 * An empty string (default) means that activate labels are not produced.
 * |default ""
 * |widget StringEntry()
 * |preview valid
 * |tab Labels
 *
 * |param deactivationId[Deactivation ID] The label ID to mark the element that crosses the deactivation threshold (when active).
 * An empty string (default) means that deactivate labels are not produced.
 * |default ""
 * |widget StringEntry()
 * |preview valid
 * |tab Labels
 *
 * |factory /comms/energy_detector(dtype)
 * |setter setForwardMode(forwardMode)
 * |setter setActivationLevel(activationLevel)
 * |setter setActivationMinimum(activationMin)
 * |setter setActivationMaximum(activationMax)
 * |setter setDeactivationLevel(deactivationLevel)
 * |setter setDeactivationMinimum(deactivationMin)
 * |setter setDeactivationMaximum(deactivationMax)
 * |setter setSignalAverage(signalAverage)
 * |setter setNoiseAverage(noiseAverage)
 * |setter setLookahead(lookahead)
 * |setter setActivationId(activationId)
 * |setter setDeactivationId(deactivationId)
 **********************************************************************/
template <typename Type, typename RealType>
class EnergyDetector : public Pothos::Block
{
public:
    EnergyDetector(void)
    {
        this->setupInput(0, typeid(Type));
        this->setupOutput(0, typeid(Type), this->uid()); //unique domain because of buffer forwarding

        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setForwardMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getForwardMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setActivationLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getActivationLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setActivationMinimum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getActivationMinimum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setActivationMaximum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getActivationMaximum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setDeactivationLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getDeactivationLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setDeactivationMinimum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getDeactivationMinimum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setDeactivationMaximum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getDeactivationMaximum));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setSignalAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getSignalAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setNoiseAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getNoiseAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setLookahead));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getLookahead));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setActivationId));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getActivationId));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setDeactivationId));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getDeactivationId));

        //defaults
        this->setForwardMode("ACTIVE");
        this->setActivationLevel(3.0);
        this->setActivationMinimum(0);
        this->setActivationMaximum(0);
        this->setDeactivationLevel(3.0);
        this->setDeactivationMinimum(0);
        this->setDeactivationMaximum(0);
        this->setSignalAverage(10);
        this->setNoiseAverage(100);
        this->setLookahead(10);
        this->setActivationId("");
        this->setDeactivationId("");
    }

    void setForwardMode(const std::string &mode)
    {
        if (mode == "ALL") _dropInactiveSamples = false;
        else if (mode == "ACTIVE") _dropInactiveSamples = true;
        else throw Pothos::InvalidArgumentException("EnergyDetector::setForwardMode("+mode+")", "unknown mode");
        _forwardMode = mode;
    }

    std::string getForwardMode(void)
    {
        return _forwardMode;
    }

    void setActivationLevel(const double level)
    {
        _activationLeveldB = level;
        _activationFactor = RealType(std::pow(10.0, _activationLeveldB/20));
    }

    double getActivationLevel(void) const
    {
        return _activationLeveldB;
    }

    void setActivationMinimum(const size_t minimum)
    {
        _activationMinimum = minimum;
    }

    size_t getActivationMinimum(void) const
    {
        return _activationMinimum;
    }

    void setActivationMaximum(const size_t maximum)
    {
        _activationMaximum = maximum;
    }

    size_t getActivationMaximum(void) const
    {
        return _activationMaximum;
    }

    void setDeactivationLevel(const double level)
    {
        _deactivationLeveldB = level;
        _deactivationFactor = RealType(std::pow(10.0, _deactivationLeveldB/20));
    }

    double getDeactivationLevel(void) const
    {
        return _deactivationLeveldB;
    }

    void setDeactivationMinimum(const size_t minimum)
    {
        _deactivationMinimum = minimum;
    }

    size_t getDeactivationMinimum(void) const
    {
        return _deactivationMinimum;
    }

    void setDeactivationMaximum(const size_t maximum)
    {
        _deactivationMaximum = maximum;
    }

    size_t getDeactivationMaximum(void) const
    {
        return _deactivationMaximum;
    }

    void setSignalAverage(const double signalAvg)
    {
        _signalAverage = signalAvg;
        _signalGain = RealType(std::exp(-1/signalAvg));
        _oneMinusSignalGain = 1-_signalGain;
    }

    double getSignalAverage(void) const
    {
        return _signalAverage;
    }

    void setNoiseAverage(const double noiseAvg)
    {
        _noiseAverage = noiseAvg;
        _noiseGain = RealType(std::exp(-1/noiseAvg));
        _oneMinusNoiseGain = 1-_noiseGain;
    }

    double getNoiseAverage(void) const
    {
        return _noiseAverage;
    }

    void setLookahead(const size_t lookahead)
    {
        _lookahead = lookahead;
    }

    size_t getLookahead(void) const
    {
        return _lookahead;
    }

    void setActivationId(const std::string &id)
    {
        _activationId = id;
    }

    std::string getActivationId(void) const
    {
        return _activationId;
    }

    void setDeactivationId(const std::string &id)
    {
        _deactivationId = id;
    }

    std::string getDeactivationId(void) const
    {
        return _deactivationId;
    }

    void activate(void)
    {
        //reset state before running
        _activeState = false;
        _samplesInState = 0;
        _currentSignalEnvelope = 0;
        _currentNoiseEnvelope = 0;
    }

    void work(void)
    {
        //access ports
        auto inPort = this->input(0);
        auto outPort = this->output(0);

        //ensure that the input has lookahead room
        if (inPort->elements() <= _lookahead)
        {
            inPort->setReserve(_lookahead+1);
            return;
        }

        //calculate the work size given the available resources
        const size_t N = std::min(inPort->elements()-_lookahead, outPort->elements());
        if (N == 0) return;

        size_t i = 0;
        const bool entryState = _activeState;
        const auto in = inPort->buffer().template as<const Type *>();

        std::cout << "_activeState " << _activeState << std::endl;
        std::cout << "_currentSignalEnvelope " << 20*std::log10(_currentSignalEnvelope) << std::endl;
        std::cout << "_currentNoiseEnvelope " << 20*std::log10(_currentNoiseEnvelope) << std::endl;
        std::cout << "_activationLevel " << 20*std::log10(_activationFactor) << std::endl;
        std::cout << "_deactivationLevel " << 20*std::log10(_deactivationFactor) << std::endl;
        std::cout << "_signalGain " << _signalGain << std::endl;
        std::cout << "_noiseGain " << _noiseGain << std::endl;
        std::cout << "_samplesInState " << _samplesInState << std::endl;

        if (_activeState)
        {
            //create activation label on the first sample in this state
            if (_samplesInState == 0 and not _activationId.empty())
            {
                outPort->postLabel(Pothos::Label(_activationId, Pothos::Object(), 0));
            }

            for (i = 0; i < N; i++)
            {
                _samplesInState++;
                const RealType xn = RealType(std::abs(in[i+_lookahead]));
                _currentSignalEnvelope = _signalGain*_currentSignalEnvelope + _oneMinusSignalGain*xn;

                //deactivate when the envelope drops below the deactivation threshold
                //and the sample minimum is met, or when the maximum has been reached
                if (
                    ((_samplesInState > _activationMinimum) and (_currentSignalEnvelope < _currentNoiseEnvelope*_deactivationFactor)) or
                    (_activationMaximum != 0 and _samplesInState > _activationMaximum)
                )
                {

                    //create deactivation label on the last sample in this state
                    if (not _deactivationId.empty())
                    {
                        outPort->postLabel(Pothos::Label(_deactivationId, Pothos::Object(), i));
                    }

                    _samplesInState = 0;
                    _activeState = not _activeState;
                    break;
                }
            }
        }
        else
        {
            for (i = 0; i < N; i++)
            {
                _samplesInState++;
                const RealType xn = RealType(std::abs(in[i+_lookahead]));
                _currentSignalEnvelope = _signalGain*_currentSignalEnvelope + _oneMinusSignalGain*xn;
                _currentNoiseEnvelope = _noiseGain*_currentNoiseEnvelope + _oneMinusNoiseGain*xn;

                //activate when the envelope rises above the activation threshold
                //and the sample minimum is met, or when the maximum has been reached
                if (
                    ((_samplesInState > _deactivationMinimum) and (_currentSignalEnvelope > _currentNoiseEnvelope*_activationFactor)) or
                    (_deactivationMaximum != 0 and _samplesInState > _deactivationMaximum)
                )
                {
                    _samplesInState = 0;
                    _activeState = not _activeState;
                    break;
                }
            }
        }

        //consume input
        inPort->consume(i+1);

        //forward buffer for active work() or when forwarding inactive samples
        if (entryState or not _dropInactiveSamples)
        {
            auto buff = inPort->buffer();
            buff.length = (i+1)*sizeof(Type);
            outPort->postBuffer(buff);
        }
    }

private:

    //current processing state
    bool _activeState;
    bool _dropInactiveSamples;
    RealType _currentSignalEnvelope;
    RealType _currentNoiseEnvelope;
    RealType _signalGain;
    RealType _oneMinusSignalGain;
    RealType _noiseGain;
    RealType _oneMinusNoiseGain;
    RealType _activationFactor;
    RealType _deactivationFactor;
    unsigned long long _samplesInState;

    //configuration params
    std::string _forwardMode;
    double _activationLeveldB;
    size_t _activationMinimum;
    size_t _activationMaximum;
    double _deactivationLeveldB;
    size_t _deactivationMinimum;
    size_t _deactivationMaximum;
    double _signalAverage;
    double _noiseAverage;
    size_t _lookahead;
    std::string _activationId;
    std::string _deactivationId;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *valueProbeFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(type))) return new EnergyDetector<type, type>(); \
        if (dtype == Pothos::DType(typeid(std::complex<type>))) return new EnergyDetector<std::complex<type>, type>();
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    //ifTypeDeclareFactory(int64_t);
    //ifTypeDeclareFactory(int32_t);
    //ifTypeDeclareFactory(int16_t);
    //ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("valueProbeFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerEnergyDetector(
    "/comms/energy_detector", &valueProbeFactory);
