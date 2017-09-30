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
    waveSource.call("setWaveform", "SINE");
    waveSource.call("setFrequency", waveFreq);
    waveSource.call("setSampleRate", sampRate);

    auto finiteRelease = Pothos::BlockRegistry::make("/blocks/finite_release");
    finiteRelease.call("setTotalElements", 4096);

    auto filter = Pothos::BlockRegistry::make("/comms/iir_filter", "complex128");
		filter.call("setWaitTaps", true);

    auto designer = Pothos::BlockRegistry::make("/comms/iir_designer");
    designer.call("setSampleRate", sampRate);
    designer.call("setIIRType", "butterworth");
    designer.call("setFilterType", "LOW_PASS");
    designer.call("setFrequencyLower", 0.1*sampRate);
    designer.call("setOrder", 4);

    auto probe = Pothos::BlockRegistry::make("/comms/signal_probe", "complex128");
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

POTHOS_TEST_BLOCK("/comms/tests", test_iir_filter)
{

	const double rate = 1e6;
	const double freq = 30e3;
	std::cout << "freq " << freq << std::flush;
	auto rms = iirfilterToneGetRMS(rate, freq);
	std::cout << " RMS = " << rms << std::endl;
	POTHOS_TEST_TRUE(rms > 0.1);
}
