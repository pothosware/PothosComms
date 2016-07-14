// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <cmath>
#include <complex>
#include <cstdint>

/***********************************************************************
 * Templated angle/atan2 for fixed and float support
 **********************************************************************/
uint16_t fxpt_atan2(const int16_t y, const int16_t x);

template <typename Type>
typename std::enable_if<std::is_floating_point<Type>::value, Type>::type
getAngle(const std::complex<Type> &in)
{
    return std::arg(in);
}

template <typename Type>
typename std::enable_if<std::is_integral<Type>::value, Type>::type
getAngle(const std::complex<Type> &in)
{
    const auto real16 = int16_t(in.real());
    const auto imag16 = int16_t(in.imag());
    const auto u16out = fxpt_atan2(imag16, real16);
    return Type(u16out);
}

/***********************************************************************
 * Templated abs/magnitude for fixed and float support
 **********************************************************************/

//absolute value for floating point and real integer types
template <typename OutType, typename InType>
OutType getAbs(const InType &in)
{
    return OutType(std::abs(in));
}

//absolute value for fixed point complex types
template <typename OutType, typename InType>
typename std::enable_if<std::is_integral<OutType>::value, OutType>::type
getAbs(const std::complex<InType> &in)
{
    const auto mag2 = in.real()*in.real() + in.imag()*in.imag();
    return OutType(std::sqrt(float(mag2)));
}
