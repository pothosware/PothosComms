// Copyright (c) 2024 Sean Nowlan
// Copyright (c) 2014-2019 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <cmath> //abs
#include <complex>

template <typename T>
T getMin(const T& a, const T& b) {
    return std::min<T>(a, b);
}

template <typename T>
T getMax(const T& a, const T& b) {
    return std::max<T>(a, b);
}

template <typename T>
std::complex<T> getMin(const std::complex<T>& a, const std::complex<T>& b) {
    return std::min<T>(std::abs(a), std::abs(b));
}

template <typename T>
std::complex<T> getMax(const std::complex<T>& a, const std::complex<T>& b) {
    return std::max<T>(std::abs(a), std::abs(b));
}
