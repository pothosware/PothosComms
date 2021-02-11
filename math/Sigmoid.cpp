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

template <typename Type>
using SigmoidFcn = void(*)(const Type*, Type*, const size_t);

#ifdef POTHOS_XSIMD

template <typename Type>
static inline SigmoidFcn<Type> getSigmoidFcn()
{
    return PothosCommsSIMD::sigmoidDispatch<Type>();
}

#else

template <typename Type>
static inline SigmoidFcn<Type> getSigmoidFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++)
        {
            out[i] = Type(1.0) / (Type(1.0) + std::exp(-in[i]));
        }
    };
}

#endif

/***********************************************************************
 * |PothosDoc Sigmoid
 *
 * Perform the sigmoid function on all inputs, defined as:
 *
 * <p><b>y = 1 / 1 + e(-x)</b></p>
 *
 * |category /Math
 * |keywords math sin
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/sigmoid(dtype)
 **********************************************************************/
template <typename Type>
class Sigmoid : public Pothos::Block
{
public:
    using Class = Sigmoid<Type>;

    Sigmoid(const size_t dimension): _fcn(getSigmoidFcn<Type>())
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

        const size_t N = elems*inPort->dtype().dimension();
        _fcn(in, out, N);

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    SigmoidFcn<Type> _fcn;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block* sigmoidFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Sigmoid<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    throw Pothos::InvalidArgumentException("sigmoidFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerSigmoid(
    "/comms/sigmoid",
    Pothos::Callable(&sigmoidFactory));
