// Copyright (c) 2015-2015 Tony Kirke
// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cmath> //fabs
#include <iostream>

static double iirfilterToneGetRMS(
																	const double sampRate,
																	const double waveFreq
)
{
    auto waveSource = Pothos::BlockRegistry::make("/comms/waveform_source", "complex128");
    waveSource.callVoid("setWaveform", "SINE");
    waveSource.callVoid("setFrequency", waveFreq);
    waveSource.callVoid("setSampleRate", sampRate);

    auto finiteRelease = Pothos::BlockRegistry::make("/blocks/finite_release");
    finiteRelease.callVoid("setTotalElements", 4096);

    auto filter = Pothos::BlockRegistry::make("/comms/iir_filter", "complex128");
		filter.callVoid("setWaitTaps", true);

    auto designer = Pothos::BlockRegistry::make("/comms/iir_designer");
    designer.callVoid("setSampleRate", sampRate);
    designer.callVoid("setIIRType", "butterworth");
    designer.callVoid("setFilterType", "LOW_PASS");
    designer.callVoid("setFrequencyLower", 0.1*sampRate);
    designer.callVoid("setOrder", 4);

    auto probe = Pothos::BlockRegistry::make("/comms/signal_probe", "complex128");
    probe.callVoid("setMode", "RMS");

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

POTHOS_TEST_BLOCK("/comms/tests", test_iir_filter)
{

	const double rate = 1e6;
	const double freq = 30e3;
	std::cout << "freq " << freq << std::flush;
	auto rms = iirfilterToneGetRMS(rate, freq);
	std::cout << " RMS = " << rms << std::endl;
	POTHOS_TEST_TRUE(rms > 0.1);
}
