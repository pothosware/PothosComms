// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
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
 * |param activationLevel[Activation Level] The threshold level that the input must exceed to activate.
 * This threshold level is a power in dB that is relative to the noise floor.
 * |default 3.0
 * |units dB
 * |tab EnergyDetectors
 *
 * |param activationMin[Activation Minimum] The minimum number of samples to remain activated.
 * The detector will remain active for at least this many samples reguardless of the deactivation threshold.
 * Once the minimum number of samples has been reached, the deactivation threshold comes into play.
 * |default 0
 * |preview valid
 * |units samples
 * |tab EnergyDetectors
 *
 * |param activationMax[Activation Maximum] The maximum number of samples to remain activated.
 * The detector will remain active for at most many samples reguardless of the deactivation threshold.
 * Once the maximum number of samples has been reached, the deactivation state is forced.
 * A maximum of 0 means that no maximum will be enforced.
 * |default 0
 * |preview valid
 * |units samples
 * |tab EnergyDetectors
 *
 * |param deactivationLevel[Deactivation Level] The threshold level that the input must fall-below to deactivate.
 * This threshold level is a power in dB that is relative to the noise floor.
 * |default 3.0
 * |units dB
 * |tab EnergyDetectors
 *
 * |param deactivationMin[Deactivation Minimum] The minimum number of samples to remain inactive.
 * The detector will remain inactive for at least this many samples reguardless of the activation threshold.
 * Once the minimum number of samples has been reached, the activation threshold comes into play.
 * |default 0
 * |preview valid
 * |units samples
 * |tab EnergyDetectors
 *
 * |param deactivationMax[Deactivation Maximum] The maximum number of samples to remain inactive.
 * The detector will remain inactive for at most many samples reguardless of the activation threshold.
 * Once the maximum number of samples has been reached, the activation state is forced.
 * A maximum of 0 means that no maximum will be enforced.
 * |default 0
 * |preview valid
 * |units samples
 * |tab EnergyDetectors
 *
 * |param attackAverage[Attack Average] The run-up time constant in samples.
 * This parameter effectively controls the averaging detector while inactive.
 * Single pole filter roll-off constant: gainAttackAverage = exp(-1/attackAverage)
 * |default 10
 * |units samples
 * |tab Filter
 *
 * |param releaseAverage[Release Average] The decay time constant in samples.
 * This parameter effectively controls the averaging detector while active.
 * Single pole filter roll-off constant: gainReleaseAverage = exp(-1/releaseAverage)
 * |default 10
 * |units samples
 * |tab Filter
 *
 * |param noiseAverage[Noise Average] The noise averager decay time constant in samples.
 * The detector uses this filter to determine the noise floor when inactive.
 * Single pole filter roll-off constant: gainNoiseAverage = exp(-1/noiseAverage)
 * |default 100
 * |units samples
 * |tab Filter
 *
 * |param lookahead A configurable input delay to compensate for envelope lag.
 * Without lookahead, the envelope calculation lags behind the input due to filtering.
 * The lookahead compensation adjusts the envelope to match up with the input events.
 * |default 10
 * |units samples
 * |tab Filter
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
 * |setter setAttackAverage(attackAverage)
 * |setter setReleaseAverage(releaseAverage)
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
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setAttackAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getAttackAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, setReleaseAverage));
        this->registerCall(this, POTHOS_FCN_TUPLE(EnergyDetector, getReleaseAverage));
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
        this->setAttackAverage(10);
        this->setReleaseAverage(10);
        this->setNoiseAverage(100);
        this->setLookahead(10);
        this->setActivationId("");
        this->setDeactivationId("");
    }

    void setForwardMode(const std::string &mode)
    {
        //TODO set bool flag
        if (mode == "ALL") {}
        else if (mode == "ACTIVE") {}
        else throw Pothos::InvalidArgumentException("EnergyDetector::setForwardMode("+mode+")", "unknown mode");
        _forwardMode = mode;
    }

    std::string getForwardMode(void)
    {
        return _forwardMode;
    }

    void setActivationLevel(const RealType level)
    {
        _activationLevel = level;
    }

    RealType getActivationLevel(void) const
    {
        return _activationLevel;
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

    void setDeactivationLevel(const RealType level)
    {
        _deactivationLevel = level;
    }

    RealType getDeactivationLevel(void) const
    {
        return _deactivationLevel;
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

    void setAttackAverage(const RealType attackAvg)
    {
        _attackAverage = attackAvg;
        //_attackGain = std::exp(-1/attack);
        //_oneMinusAttackGain = 1-_attackGain;
    }

    RealType getAttackAverage(void) const
    {
        return _attackAverage;
    }

    void setReleaseAverage(const RealType releaseAvg)
    {
        _releaseAverage = releaseAvg;
        //_releaseGain = std::exp(-1/release);
        //_oneMinusReleaseGain = 1-_releaseGain;
    }

    RealType getReleaseAverage(void) const
    {
        return _releaseAverage;
    }

    void setNoiseAverage(const RealType noiseAvg)
    {
        _noiseAverage = noiseAvg;
    }

    RealType getNoiseAverage(void) const
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
        
    }

    void work(void)
    {
        
    }

private:

    //current processing state


    //configuration params
    std::string _forwardMode;
    RealType _activationLevel;
    size_t _activationMinimum;
    size_t _activationMaximum;
    RealType _deactivationLevel;
    size_t _deactivationMinimum;
    size_t _deactivationMaximum;
    RealType _attackAverage;
    RealType _releaseAverage;
    RealType _noiseAverage;
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
        if (dtype == Pothos::DType(typeid(type))) return new EnergyDetector<type, double>(); \
        if (dtype == Pothos::DType(typeid(std::complex<type>))) return new EnergyDetector<std::complex<type>, std::complex<double>>();
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("valueProbeFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerEnergyDetector(
    "/comms/energy_detector", &valueProbeFactory);
