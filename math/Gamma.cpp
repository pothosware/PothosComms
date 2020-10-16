// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cmath>

//
// Implementation getters to be called on class construction
//

template <typename Type>
using GammaFcn = void(*)(const Type*, Type*, size_t);

template <typename Type>
static inline GammaFcn<Type> getGammaFcn()
{
    return [](const Type* in, Type* out, size_t len)
    {
        for (size_t i = 0; i < len; ++i) out[i] = std::tgamma(in[i]);
    };
}

template <typename Type>
static inline GammaFcn<Type> getLnGammaFcn()
{
    return [](const Type* in, Type* out, size_t len)
    {
        for (size_t i = 0; i < len; ++i) out[i] = std::lgamma(in[i]);
    };
}

//
// Block implementation
//

template <typename Type>
class Gamma : public Pothos::Block
{
public:
    Gamma(const size_t dimension, GammaFcn<Type> fcn): _fcn(fcn)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        auto input = this->input(0);
        auto output = this->output(0);

        const size_t N = elems * input->dtype().dimension();
        _fcn(input->buffer(), output->buffer(), N);

        input->consume(elems);
        output->produce(elems);
    }

private:
    GammaFcn<Type> _fcn;
};

//
// Registration
//

static Pothos::Block *gammaFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareGammaFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Gamma<type>(dtype.dimension(), getGammaFcn<type>());
    ifTypeDeclareGammaFactory(double);
    ifTypeDeclareGammaFactory(float);

    throw Pothos::InvalidArgumentException("gammaFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::Block* lnGammaFactory(const Pothos::DType& dtype)
{
    #define ifTypeDeclareLnGammaFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Gamma<type>(dtype.dimension(), getLnGammaFcn<type>());
    ifTypeDeclareLnGammaFactory(double);
    ifTypeDeclareLnGammaFactory(float);

    throw Pothos::InvalidArgumentException("lnGammaFactory(" + dtype.toString() + ")", "unsupported type");
}

//
// Factories
//

/***********************************************************************
 * |PothosDoc Gamma
 *
 * Perform the gamma function on every input element.
 *
 * out[n] = gamma(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/gamma(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerGamma(
    "/comms/gamma",
    &gammaFactory);

/***********************************************************************
 * |PothosDoc Log Gamma
 *
 * Calculate the natural log of the result of performing the gamma
 * function on every input element.
 *
 * out[n] = ln(gamma(in[n]))
 *
 * |category /Math
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/lngamma(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerLnGamma(
    "/comms/lngamma",
    &lnGammaFactory);
