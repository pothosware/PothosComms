// Copyright (c) 2015-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max
#include <type_traits>

//
// Implementation getters to be called on class construction
//

template <typename Type>
using ConjFcn = void(*)(const Type*, Type*, const size_t);

#ifdef POTHOS_XSIMD

// Due to a limitation in XSIMD's AVX implementation, there is no int16_t SIMD
// implementation, so go with the default implementation.
template <typename Type>
struct HasSIMDImplementation : std::integral_constant<
    bool,
    !std::is_same<Type, std::complex<int16_t>>::value>
{};

template <typename Type>
static inline typename std::enable_if<HasSIMDImplementation<Type>::value, ConjFcn<Type>>::type getConjFcn()
{
    return PothosCommsSIMD::conjDispatch<Type>();
}

template <typename Type>
static inline typename std::enable_if<!HasSIMDImplementation<Type>::value, ConjFcn<Type>>::type
#else
template <typename Type>
static inline ConjFcn<Type>
#endif
getConjFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::conj(in[i]);
    };
}

/***********************************************************************
 * |PothosDoc Conjugate
 *
 * Take the complex conjugate of every input element.
 *
 * out[n] = conj(in[n])
 *
 * |category /Math
 * |keywords math conjugate complex conj
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(cfloat=1,cint=1,dim=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |factory /comms/conjugate(dtype)
 **********************************************************************/
template <typename Type>
class Conjugate : public Pothos::Block
{
public:
    Conjugate(const size_t dimension)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //get pointers to in and out buffer
        auto inPort = this->input(0);
        auto outPort = this->output(0);
        const Type *in = inPort->buffer();
        Type *out = outPort->buffer();

        //perform conjugate operation
        const size_t N = elems*inPort->dtype().dimension();
        _fcn(in, out, N);

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    static ConjFcn<Type> _fcn;
};

template <typename Type>
ConjFcn<Type> Conjugate<Type>::_fcn = getConjFcn<Type>();

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *conjugateFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory_(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Conjugate<type>(dtype.dimension());
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(std::complex<type>)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("conjugateFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerConjugate(
    "/comms/conjugate", &conjugateFactory);
