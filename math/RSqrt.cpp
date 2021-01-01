// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#else
#include "RSqrt.hpp"
#endif

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

//
// Templated getter to be called on class construction
//

template <typename T>
using RSqrtFcn = void(*)(const T*, T*, size_t);

#ifdef POTHOS_XSIMD

template <typename T>
static RSqrtFcn<T> getRSqrtFcn()
{
    return PothosCommsSIMD::rsqrtDispatch<T>();
}

#else

template <typename T>
static RSqrtFcn<T> getRSqrtFcn()
{
    return &rsqrtBuffer<T>;
}

#endif

//
// Block implementation
//

template <typename T>
class RSqrt: public Pothos::Block
{
    public:
        RSqrt(size_t dimension)
        {
            const Pothos::DType dtype(typeid(T), dimension);

            this->setupInput(0, dtype);
            this->setupOutput(0, dtype);
        }

        virtual ~RSqrt() = default;

        void work() override
        {
            const auto elems = this->workInfo().minElements;
            if(0 == elems) return;

            auto* inPort = this->input(0);
            auto* outPort = this->output(0);

            const auto N = elems * inPort->dtype().dimension();
            _fcn(inPort->buffer(), outPort->buffer(), N);

            inPort->consume(elems);
            outPort->produce(elems);
        }

    private:
        static RSqrtFcn<T> _fcn;
};

template <typename T>
RSqrtFcn<T> RSqrt<T>::_fcn = getRSqrtFcn<T>();

//
// Factory
//

/***********************************************************************
 * |PothosDoc Reciprocal Square Root
 *
 * Calculate the reciprocal square root of each input element.
 *
 * out[n] = 1 / sqrt(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/rsqrt(dtype)
 **********************************************************************/
static Pothos::Block* makeRSqrt(const Pothos::DType& dtype)
{
    #define ifTypeThenReturn(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new RSqrt<T>(dtype.dimension());

    ifTypeThenReturn(float)
    ifTypeThenReturn(double)

    throw Pothos::InvalidArgumentException("Unsupported dtype: "+dtype.toString());
}

//
// Registration
//

static Pothos::BlockRegistry registerRSqrt(
    "/comms/rsqrt",
    &makeRSqrt);
