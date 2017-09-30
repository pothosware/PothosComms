// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cmath> //fabs
#include <iostream>

static double filterToneGetRMS(
    const Pothos::DType &dtype,
    const double amplitude,
    const double sampRate,
    const double waveFreq,
    const size_t decim,
    const size_t interp
)
{
    auto waveSource = Pothos::BlockRegistry::make("/comms/waveform_source", dtype);
    waveSource.call("setAmplitude", amplitude);
    waveSource.call("setWaveform", "SINE");
    waveSource.call("setFrequency", waveFreq);
    waveSource.call("setSampleRate", sampRate);

    auto finiteRelease = Pothos::BlockRegistry::make("/blocks/finite_release");
    finiteRelease.call("setTotalElements", 4096);

    auto filter = Pothos::BlockRegistry::make("/comms/fir_filter", dtype, "COMPLEX");
    filter.call("setDecimation", decim);
    filter.call("setInterpolation", interp);
    filter.call("setWaitTaps", true);

    auto designer = Pothos::BlockRegistry::make("/comms/fir_designer");
    designer.call("setSampleRate", (sampRate*interp)/decim);
    designer.call("setFilterType", "SINC");
    designer.call("setBandType", "COMPLEX_BAND_PASS");
    designer.call("setFrequencyLower", waveFreq-0.1*sampRate);
    designer.call("setFrequencyUpper", waveFreq+0.1*sampRate);
    designer.call("setBandwidthTrans", waveFreq+0.1*sampRate);
    designer.call("setNumTaps", 101);

    auto probe = Pothos::BlockRegistry::make("/comms/signal_probe", dtype);
    probe.call("setMode", "RMS");

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(designer, "tapsChanged", filter, "setTaps");
        topology.connect(waveSource, 0, finiteRelease, 0);
        topology.connect(finiteRelease, 0, filter, 0);
        topology.connect(filter, 0, probe, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    return probe.call<double>("value");
}

POTHOS_TEST_BLOCK("/comms/tests", test_fir_filter)
{
    std::vector<Pothos::DType> types;
    types.push_back(Pothos::DType("complex_float64"));
    types.push_back(Pothos::DType("complex_int16"));

    for (size_t i = 0; i < types.size(); i++)
    {
        std::cout << "Testing FIR filter on data type " << types[i].toString() << std::endl;
        for (size_t decim = 1; decim <= 3; decim++)
        {
            for (size_t interp = 1; interp <= 3; interp++)
            {
                const double amplitude = 1000;
                const double rate = 1e6;
                const double freq = 30e3;
                std::cout << "freq " << freq << " decim " << decim << " interp " << interp << std::flush;
                auto rms = filterToneGetRMS(types[i], amplitude, rate, freq, decim, interp);
                std::cout << " RMS = " << rms << std::endl;
                POTHOS_TEST_TRUE(rms > (0.1*amplitude));
            }
        }
    }
}
