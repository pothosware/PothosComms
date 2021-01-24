// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Framework.hpp>

#include <cmath>
#include <type_traits>

//
// Implementation getters to be called on class construction
//

template <typename Type>
using PowFcn = void(*)(const Type*, Type*, Type, size_t);

#ifdef POTHOS_XSIMD

template <typename Type>
static inline typename std::enable_if<std::is_floating_point<Type>::value, PowFcn<Type>>::type getPowFcn()
{
    return PothosCommsSIMD::powDispatch<Type>();
}

template <typename Type>
static inline typename std::enable_if<!std::is_floating_point<Type>::value, PowFcn<Type>>::type
#else
template <typename Type>
static inline PowFcn<Type>
#endif
getPowFcn()
{
    return [](const Type* in, Type* out, Type exponent, size_t num)
    {
        for (size_t i = 0; i < num; i++)
        {
            out[i] = Type(std::pow(in[i], exponent));
        }
    };
}

template <typename Type>
struct NeedToValidateExponent : std::integral_constant<bool,
                                    std::is_signed<Type>::value &&
                                    !std::is_floating_point<Type>::value> {};

/***********************************************************************
 * |PothosDoc Pow
 *
 * Raise each input to a given exponent.
 *
 * |category /Math
 * |keywords exponent
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(int=1,uint=1,float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param exponent[Exponent] The exponent to which to raise each input.
 * |widget SpinBox()
 * |default 0
 * |preview enable
 *
 * |factory /comms/pow(dtype,exponent)
 * |setter setExponent(exponent)
 **********************************************************************/
template <typename Type>
class Pow : public Pothos::Block
{
public:
    using Class = Pow<Type>;

    Pow(const size_t dimension, Type exponent)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, exponent));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setExponent));
        this->registerProbe("exponent");
        this->registerSignal("exponentChanged");

        this->setExponent(exponent);

        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    virtual ~Pow() = default;

    Type exponent() const
    {
        return _exponent;
    }

    void setExponent(Type exponent)
    {
        this->validateExponentIfNeeded(exponent);
        _exponent = exponent;

        this->emitSignal("exponentChanged");
    }

    void work(void)
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

        auto* input = this->input(0);
        auto* output = this->output(0);

        const auto N = elems * input->dtype().dimension();
        _fcn(input->buffer(), output->buffer(), _exponent, N);

        input->consume(elems);
        output->produce(elems);
    }

private:
    static PowFcn<Type> _fcn;

    Type _exponent;

    template <typename Type2 = Type>
    static typename std::enable_if<NeedToValidateExponent<Type2>::value>::type validateExponentIfNeeded(Type2 val)
    {
        if (val < 0)
        {
            throw Pothos::InvalidArgumentException("Cannot use this exponent with this type, as the output cannot be represented.");
        }
    }

    template <typename Type2 = Type>
    static typename std::enable_if<!NeedToValidateExponent<Type2>::value>::type validateExponentIfNeeded(Type2)
    {
        // noop
    }
};

template <typename Type>
PowFcn<Type> Pow<Type>::_fcn = getPowFcn<Type>();

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block* powFactory(
    const Pothos::DType &dtype,
    const Pothos::Object& exponent)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Pow<type>(dtype.dimension(), exponent.convert<type>());
    ifTypeDeclareFactory(std::int8_t);
    ifTypeDeclareFactory(std::int16_t);
    ifTypeDeclareFactory(std::int32_t);
    ifTypeDeclareFactory(std::int64_t);
    ifTypeDeclareFactory(std::uint8_t);
    ifTypeDeclareFactory(std::uint16_t);
    ifTypeDeclareFactory(std::uint32_t);
    ifTypeDeclareFactory(std::uint64_t);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(double);
    throw Pothos::InvalidArgumentException("powFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerPow(
    "/comms/pow",
    Pothos::Callable(&powFactory));
