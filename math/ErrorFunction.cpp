// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Framework.hpp>

#include <cmath>

//
// Implementation getters to be called on class construction
//

#ifdef POTHOS_XSIMD

template <typename Type>
using ErfFcn = void(*)(const Type*, Type*, size_t);

template <typename Type>
static inline ErfFcn<Type> getErfFcn()
{
    return PothosCommsSIMD::erfDispatch<Type>();
}

template <typename Type>
static inline ErfFcn<Type> getErfcFcn()
{
    return PothosCommsSIMD::erfcDispatch<Type>();
}

#else

template <typename Type>
using ErfFcn = void(*)(const Type*, Type*, size_t);

template <typename Type>
static inline ErfFcn<Type> getErfFcn()
{
    return [](const Type* in, Type* out, size_t len)
    {
        for (size_t i = 0; i < len; ++i) out[i] = std::erf(in[i]);
    };
}

template <typename Type>
static inline ErfFcn<Type> getErfcFcn()
{
    return [](const Type* in, Type* out, size_t len)
    {
        for (size_t i = 0; i < len; ++i) out[i] = std::erfc(in[i]);
    };
}

#endif

//
// Block implementation
//

template <typename Type>
class ErrorFunction : public Pothos::Block
{
public:
    ErrorFunction(const size_t dimension, ErfFcn<Type> fcn): _fcn(fcn)
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
    ErfFcn<Type> _fcn;
};

//
// Registration
//

static Pothos::Block* erfFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareErfFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new ErrorFunction<type>(dtype.dimension(), getErfFcn<type>());
    ifTypeDeclareErfFactory(double);
    ifTypeDeclareErfFactory(float);

    throw Pothos::InvalidArgumentException("erfFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::Block* erfcFactory(const Pothos::DType& dtype)
{
    #define ifTypeDeclareErfcFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new ErrorFunction<type>(dtype.dimension(), getErfcFcn<type>());
    ifTypeDeclareErfcFactory(double);
    ifTypeDeclareErfcFactory(float);

    throw Pothos::InvalidArgumentException("erfcFactory(" + dtype.toString() + ")", "unsupported type");
}

//
// Factories
//

/***********************************************************************
 * |PothosDoc Error Function
 *
 * Calculate the error function for each element.
 *
 * |category /Math
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/erf(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerErf(
    "/comms/erf",
    &erfFactory);

/***********************************************************************
 * |PothosDoc Complementary Error Function
 *
 * Calculate the complementary error function for each element.
 *
 * |category /Math
 *
 * |param dtype[Data Type] The block's data type.
 * |widget DTypeChooser(float=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/erfc(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerErfc(
    "/comms/erfc",
    &erfcFactory);
