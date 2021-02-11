// Copyright (c) 2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include "Exp10.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <type_traits>

/***********************************************************************
 * Implementation getters to be called on class construction
 **********************************************************************/

// Use std::function instead of function pointer because ExpN's lambda
// needs to capture a parameter.
template <typename Type>
using ExpFcn = std::function<void(const Type*, Type*, const size_t)>;

#ifdef POTHOS_XSIMD

template <typename Type>
static inline ExpFcn<Type> getExpFcn()
{
    return PothosCommsSIMD::expDispatch<Type>();
}

template <typename Type>
static inline ExpFcn<Type> getExp2Fcn()
{
    return PothosCommsSIMD::exp2Dispatch<Type>();
}

template <typename Type>
static inline ExpFcn<Type> getExp10Fcn()
{
    return PothosCommsSIMD::exp10Dispatch<Type>();
}

template <typename Type>
static inline ExpFcn<Type> getExpM1Fcn()
{
    return PothosCommsSIMD::expm1Dispatch<Type>();
}

template <typename Type>
static inline ExpFcn<Type> getExpNFcn(Type base)
{
    using namespace std::placeholders;

    return std::bind(PothosCommsSIMD::expNDispatch<Type>(), _1, _2, base, _3);
}

#else

template <typename Type>
static inline ExpFcn<Type> getExpFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::exp(in[i]);
    };
}

template <typename Type>
static inline ExpFcn<Type> getExp2Fcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::exp2(in[i]);
    };
}

template <typename Type>
static inline ExpFcn<Type> getExp10Fcn()
{
    return &exp10Buffer<Type>;
}

template <typename Type>
static inline ExpFcn<Type> getExpM1Fcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::expm1(in[i]);
    };
}

template <typename Type>
static inline ExpFcn<Type> getExpNFcn(Type base)
{
    return [base](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::pow(base, in[i]);
    };
}

#endif

/***********************************************************************
 * Implementation
 **********************************************************************/

template <typename Type>
class Exp: public Pothos::Block
{
    public:

        using Class = Exp<Type>;
        using ClassExpFcn = ExpFcn<Type>;

        Exp(const size_t dimension, ClassExpFcn expFcn):
            _arrayExpFcn(expFcn)
        {
            this->setupInput(0, Pothos::DType(typeid(Type), dimension));
            this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
        }

        virtual ~Exp() = default;

        void work() override
        {
            const auto elems = this->workInfo().minElements;
            if(0 == elems) return;

            auto input = this->input(0);
            auto output = this->output(0);

            _arrayExpFcn(input->buffer(), output->buffer(), elems);

            input->consume(elems);
            output->produce(elems);
        }

    protected:
        ClassExpFcn _arrayExpFcn;
};

template <typename Type>
class ExpN: public Exp<Type>
{
    public:

        using Class = ExpN<Type>;
        using ClassArrayExpFcn = typename Exp<Type>::ClassExpFcn;

        ExpN(const size_t dimension, const Type base): Exp<Type>(dimension, nullptr)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, base));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setBase));
            this->registerProbe("base");
            this->registerSignal("baseChanged");

            this->setBase(base);
        }

        virtual ~ExpN() = default;

        Type base() const
        {
            return _base;
        }

        void setBase(Type base)
        {
            _base = base;

            // We can't use switch because the base can be floating-point.
            if(_base == Type(2))  this->_arrayExpFcn = getExp2Fcn<Type>();
            if(_base == Type(10)) this->_arrayExpFcn = getExp10Fcn<Type>();
            else                  this->_arrayExpFcn = getExpNFcn<Type>(_base);

            this->emitSignal("baseChanged");
        }

    protected:
        Type _base;
};

/***********************************************************************
 * Factory/registration
 **********************************************************************/

#define ifTypeDeclareExpFactory(Type, ExpFcn) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(Type))) \
        return new Exp<Type>(dtype.dimension(), ExpFcn<Type>());

#define EXPFACTORY(FactoryFcn, ExpFcn) \
    static Pothos::Block* FactoryFcn(const Pothos::DType& dtype) \
    { \
        ifTypeDeclareExpFactory(double, ExpFcn); \
        ifTypeDeclareExpFactory(float, ExpFcn); \
        ifTypeDeclareExpFactory(int64_t, ExpFcn); \
        ifTypeDeclareExpFactory(int32_t, ExpFcn); \
        ifTypeDeclareExpFactory(int16_t, ExpFcn); \
        ifTypeDeclareExpFactory(int8_t, ExpFcn); \
        ifTypeDeclareExpFactory(uint64_t, ExpFcn); \
        ifTypeDeclareExpFactory(uint32_t, ExpFcn); \
        ifTypeDeclareExpFactory(uint16_t, ExpFcn); \
        ifTypeDeclareExpFactory(uint8_t, ExpFcn); \
        throw Pothos::InvalidArgumentException( #FactoryFcn "("+dtype.toString()+")", "unsupported type"); \
    }

EXPFACTORY(expFactory,   getExpFcn)
EXPFACTORY(exp2Factory,  getExp2Fcn)
EXPFACTORY(exp10Factory, getExp10Fcn)
EXPFACTORY(expm1Factory, getExpM1Fcn)

static Pothos::Block* expNFactory(
    const Pothos::DType& dtype,
    const Pothos::Object& base)
{
    #define ifTypeDeclareExpNFactory(Type) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(Type))) \
            return new ExpN<Type>(dtype.dimension(), base.convert<Type>());
    ifTypeDeclareExpNFactory(double);
    ifTypeDeclareExpNFactory(float);
    ifTypeDeclareExpNFactory(int64_t);
    ifTypeDeclareExpNFactory(int32_t);
    ifTypeDeclareExpNFactory(int16_t);
    ifTypeDeclareExpNFactory(int8_t);
    ifTypeDeclareExpNFactory(uint64_t);
    ifTypeDeclareExpNFactory(uint32_t);
    ifTypeDeclareExpNFactory(uint16_t);
    ifTypeDeclareExpNFactory(uint8_t);
    throw Pothos::InvalidArgumentException("expNFactory("+dtype.toString()+")", "unsupported type");
}

/***********************************************************************
 * |PothosDoc Exp
 *
 * Perform e^x on every input element.
 *
 * out[n] = e^(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/exp(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerExp(
    "/comms/exp",
    Pothos::Callable(&expFactory));

/***********************************************************************
 * |PothosDoc Exp2
 *
 * Perform 2^x on every input element.
 *
 * out[n] = 2^(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/exp2(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerExp2(
    "/comms/exp2",
    Pothos::Callable(&exp2Factory));

/***********************************************************************
 * |PothosDoc Exp10
 *
 * Perform 10^x on every input element.
 *
 * out[n] = 10^(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/exp10(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerExp10(
    "/comms/exp10",
    Pothos::Callable(&exp10Factory));

/***********************************************************************
 * |PothosDoc Exp(n)-1
 *
 * Perform e^x - 1 on each element.
 *
 * out[n] = e^(in[n]) - 1
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/expm1(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerExpm1(
    "/comms/expm1",
    Pothos::Callable(&expm1Factory));

/***********************************************************************
 * |PothosDoc Exp N
 *
 * Perform the exponential function on every input element, with a given base.
 * Has optimizations for bases <b>2</b> and <b>10</b>.
 *
 * out[x] = n^(in[x])
 *
 * |category /Math
 * |setter setBase(base)
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |param base[Base] The exparithm base.
 * |widget LineEdit()
 * |default 10
 * |preview enable
 *
 * |factory /comms/expN(dtype,base)
 **********************************************************************/
static Pothos::BlockRegistry registerExpN(
    "/comms/expN",
    Pothos::Callable(&expNFactory));
