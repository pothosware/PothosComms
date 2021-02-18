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
using ModFFcn = void(*)(const Type*, Type*, Type*, const size_t);

#ifdef POTHOS_XSIMD

template <typename Type>
static inline ModFFcn<Type> getModFFcn()
{
    return PothosCommsSIMD::modfDispatch<Type>();
}

#else

template <typename Type>
static inline ModFFcn<Type> getModFFcn()
{
    return [](const Type* in, Type* integralOut, Type* fractionalOut, const size_t num)
    {
        for (size_t i = 0; i < num; i++)
        {
            fractionalOut[i] = std::modf(in[i], &integralOut[i]);
        }
    };
}

#endif

/***********************************************************************
 * |PothosDoc Decompose Floats
 *
 * Separates the integral and fractional components of the each
 * floating-point input element.
 *
 * |category /Math
 * |keywords math fractional
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/modf(dtype)
 **********************************************************************/
template <typename Type>
class ModF : public Pothos::Block
{
public:
    using Class = ModF<Type>;

    ModF(const size_t dimension)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));

        this->setupOutput("int", Pothos::DType(typeid(Type), dimension));
        this->setupOutput("frac", Pothos::DType(typeid(Type), dimension));
    }

    void work(void)
    {
        auto elems = this->workInfo().minAllElements;
        if(elems == 0) return;

        auto inPort = this->input(0);
        auto integralOutPort = this->output("int");
        auto fractionalOutPort = this->output("frac");

        const size_t N = elems * inPort->dtype().dimension();

        _fcn(
            inPort->buffer(),
            integralOutPort->buffer(),
            fractionalOutPort->buffer(),
            N);

        inPort->consume(elems);
        integralOutPort->produce(elems);
        fractionalOutPort->produce(elems);
    }

private:
    static ModFFcn<Type> _fcn;
};

template <typename Type>
ModFFcn<Type> ModF<Type>::_fcn = getModFFcn<Type>();

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block* modfFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new ModF<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    throw Pothos::InvalidArgumentException("modfFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerModF(
    "/comms/modf",
    Pothos::Callable(&modfFactory));
