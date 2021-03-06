// Copyright (c) 2019 Nick Foster
//               2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <type_traits>

// Use std::function instead of function pointer because LogN's return
// callable needs to capture a parameter.
template <typename Type>
using LogFcn = std::function<void(const Type*, Type*, const size_t)>;

#ifdef POTHOS_XSIMD

template <typename Type>
using EnableForSIMDFcn = typename std::enable_if<std::is_floating_point<Type>::value, LogFcn<Type>>::type;

template <typename Type>
using EnableForDefaultFcn = typename std::enable_if<!std::is_floating_point<Type>::value, LogFcn<Type>>::type;

#else

template <typename Type>
using EnableForDefaultFcn = LogFcn<Type>;

#endif

/***********************************************************************
 * Implementation getters to be called on class construction
 **********************************************************************/

#ifdef POTHOS_XSIMD

template <typename Type>
static inline EnableForSIMDFcn<Type> getLogFcn()
{
    return PothosCommsSIMD::logDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLog2Fcn()
{
    return PothosCommsSIMD::log2Dispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLog10Fcn()
{
    return PothosCommsSIMD::log10Dispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLog1pFcn()
{
    return PothosCommsSIMD::log1pDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLogNFcn(Type base)
{
    using namespace std::placeholders;

    return std::bind(PothosCommsSIMD::logNDispatch<Type>(), _1, _2, base, _3);
}

#endif

template <typename Type>
static inline EnableForDefaultFcn<Type> getLogFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::log(in[i]);
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLog2Fcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::log2(in[i]);
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLog10Fcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::log10(in[i]);
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLog1pFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::log1p(in[i]);
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLogNFcn(Type base)
{
    return [base](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = std::log(in[i]) / std::log(base);
    };
}

/***********************************************************************
 * Implementation
 **********************************************************************/

template <typename Type>
class Log: public Pothos::Block
{
    public:

        using Class = Log<Type>;
        using ClassLogFcn = LogFcn<Type>;

        Log(const size_t dimension, ClassLogFcn logFcn):
            _arrayLogFcn(logFcn)
        {
            this->setupInput(0, Pothos::DType(typeid(Type), dimension));
            this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
        }

        virtual ~Log() = default;

        void work() override
        {
            const auto elems = this->workInfo().minElements;
            if(0 == elems) return;

            auto input = this->input(0);
            auto output = this->output(0);

            _arrayLogFcn(input->buffer(), output->buffer(), elems);

            input->consume(elems);
            output->produce(elems);
        }

    protected:
        ClassLogFcn _arrayLogFcn;
};

template <typename Type>
class LogN: public Log<Type>
{
    public:

        using Class = LogN<Type>;
        using ClassArrayLogFcn = typename Log<Type>::ClassLogFcn;

        LogN(const size_t dimension, const Type base): Log<Type>(dimension, nullptr)
        {
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, base));
            this->registerCall(this, POTHOS_FCN_TUPLE(Class, setBase));
            this->registerProbe("base");
            this->registerSignal("baseChanged");

            this->setBase(base);
        }

        virtual ~LogN() = default;

        Type base() const
        {
            return _base;
        }

        void setBase(Type base)
        {
            if(base <= 0)
            {
                throw Pothos::RangeException("Log base must be > 0");
            }

            _base = base;

            // We can't use switch because the base can be floating-point.
            if(_base == Type(2))  this->_arrayLogFcn = getLog2Fcn<Type>();
            if(_base == Type(10)) this->_arrayLogFcn = getLog10Fcn<Type>();
            else                  this->_arrayLogFcn = getLogNFcn<Type>(_base);

            this->emitSignal("baseChanged");
        }

    protected:
        Type _base;
};

/***********************************************************************
 * Factory/registration
 **********************************************************************/

#define ifTypeDeclareLogFactory(Type, LogFcnGetter) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(Type))) \
        return new Log<Type>(dtype.dimension(), LogFcnGetter<Type>());

#define LOGFACTORY(FactoryFcn, LogFcnGetter) \
    static Pothos::Block* FactoryFcn(const Pothos::DType& dtype) \
    { \
        ifTypeDeclareLogFactory(double, LogFcnGetter); \
        ifTypeDeclareLogFactory(float, LogFcnGetter); \
        ifTypeDeclareLogFactory(int64_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(int32_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(int16_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(int8_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(uint64_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(uint32_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(uint16_t, LogFcnGetter); \
        ifTypeDeclareLogFactory(uint8_t, LogFcnGetter); \
        throw Pothos::InvalidArgumentException( #FactoryFcn "("+dtype.toString()+")", "unsupported type"); \
    }

LOGFACTORY(logFactory,   getLogFcn)
LOGFACTORY(log2Factory,  getLog2Fcn)
LOGFACTORY(log10Factory, getLog10Fcn)
LOGFACTORY(log1pFactory, getLog1pFcn)

static Pothos::Block* logNFactory(
    const Pothos::DType& dtype,
    const Pothos::Object& base)
{
    #define ifTypeDeclareLogNFactory(Type) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(Type))) \
            return new LogN<Type>(dtype.dimension(), base.convert<Type>());
    ifTypeDeclareLogNFactory(double);
    ifTypeDeclareLogNFactory(float);
    ifTypeDeclareLogNFactory(int64_t);
    ifTypeDeclareLogNFactory(int32_t);
    ifTypeDeclareLogNFactory(int16_t);
    ifTypeDeclareLogNFactory(int8_t);
    ifTypeDeclareLogNFactory(uint64_t);
    ifTypeDeclareLogNFactory(uint32_t);
    ifTypeDeclareLogNFactory(uint16_t);
    ifTypeDeclareLogNFactory(uint8_t);
    throw Pothos::InvalidArgumentException("logNFactory("+dtype.toString()+")", "unsupported type");
}

/***********************************************************************
 * |PothosDoc Log
 *
 * Perform the logarithm on every input element.
 *
 * out[n] = log(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/log(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerlog(
    "/comms/log",
    Pothos::Callable(&logFactory));

/***********************************************************************
 * |PothosDoc Log2
 *
 * Perform the base 2 logarithm on every input element.
 *
 * out[n] = log2(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/log2(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerlog2(
    "/comms/log2",
    Pothos::Callable(&log2Factory));

/***********************************************************************
 * |PothosDoc Log10
 *
 * Perform the base 10 logarithm on every input element.
 *
 * out[n] = log10(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/log10(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerlog10(
    "/comms/log10",
    Pothos::Callable(&log10Factory));

/***********************************************************************
 * |PothosDoc Log(x+1)
 *
 * Perform the log of each element, plus one.
 *
 * out[n] = log1p(in[n])
 *
 * |category /Math
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |factory /comms/log1p(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerLog1p(
    "/comms/log1p",
    Pothos::Callable(&log1pFactory));

/***********************************************************************
 * |PothosDoc Log N
 *
 * Perform the logarithm on every input element, with a given base.
 * Has optimizations for bases <b>2</b> and <b>10</b>.
 *
 * out[n] = logN(in[n])
 *
 * |category /Math
 * |setter setBase(base)
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |param base[Base] The logarithm base.
 * |widget LineEdit()
 * |default 10
 * |preview enable
 *
 * |factory /comms/logN(dtype,base)
 **********************************************************************/
static Pothos::BlockRegistry registerLogN(
    "/comms/logN",
    Pothos::Callable(&logNFactory));
