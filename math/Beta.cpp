// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cmath>

//
// Implementation getters to be called on class construction
//

template <typename Type>
using BetaFcn = void(*)(const Type*, const Type*, Type*, size_t);

template <typename Type>
static inline BetaFcn<Type> getBetaFcn()
{
    return [](const Type* in0, const Type* in1, Type* out, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
        {
#if __cplusplus >= 201703L
            out[i] = std::beta(in0[i], in1[i]);
#else
            out[i] = std::exp(std::lgamma(in0[i]) + std::lgamma(in1[i]) - std::lgamma(in0[i] + in1[i]));
#endif
        }
    };
}

//
// Block implementation
//

template <typename Type>
class Beta : public Pothos::Block
{
public:
    Beta(const size_t dimension, BetaFcn<Type> fcn) : _fcn(fcn)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupInput(1, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        auto input0 = this->input(0);
        auto input1 = this->input(1);
        auto output = this->output(0);

        const size_t N = elems * input0->dtype().dimension();
        _fcn(input0->buffer(), input1->buffer(), output->buffer(), N);

        input0->consume(elems);
        input1->consume(elems);
        output->produce(elems);
    }

private:
    BetaFcn<Type> _fcn;
};

//
// Registration
//

static Pothos::Block* betaFactory(const Pothos::DType& dtype)
{
#define ifTypeDeclareBetaFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Beta<type>(dtype.dimension(), getBetaFcn<type>());
    ifTypeDeclareBetaFactory(double);
    ifTypeDeclareBetaFactory(float);

    throw Pothos::InvalidArgumentException("betaFactory(" + dtype.toString() + ")", "unsupported type");
}

//
// Factories
//

/***********************************************************************
 * |PothosDoc Beta
 *
 * Perform the beta function on every input element in the two input
 * streams.
 *
 * out[n] = beta(in0[n], in1[n])
 *
 * |category /Math
 * |keywords euler integral gamma
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/beta(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerBeta(
    "/comms/beta",
    &betaFactory);
