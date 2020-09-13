// Copyright (c) 2019 Nick Foster
//               2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/Log_SIMDDispatcher.hpp"
#endif

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <type_traits>

template <typename Type>
using ArrayLogFcn = std::function<void(const Type*, Type*, const size_t)>;

/***********************************************************************
 * Log functions
 **********************************************************************/

#ifdef POTHOS_XSIMD

template <typename Type>
static typename std::enable_if<std::is_floating_point<Type>::value>::type arrayLog2(const Type* in, Type* out, const size_t num)
{
    // Cache on the first run.
    static auto log2Fcn = PothosCommsSIMD::log2Dispatch<Type>();

    log2Fcn(in, out, num);
}

template <typename Type>
static typename std::enable_if<!std::is_floating_point<Type>::value>::type
#else
template <typename Type>
static void
#endif
arrayLog2(const Type* in, Type* out, const size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        out[i] = std::log2(in[i]);
    }
}

template <typename Type>
static void arrayLogN(const Type *in, Type *out, Type base, const size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        out[i] = std::log(in[i]) / std::log(base);
    }
}

template <typename Type>
static void arrayLog(const Type *in, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        out[i] = std::log(in[i]);
    }
}

template <typename Type>
static void arrayLog10(const Type *in, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        out[i] = std::log10(in[i]);
    }
}

/***********************************************************************
 * Implementation
 **********************************************************************/

template <typename Type>
class Log: public Pothos::Block
{
    public:

        using Class = Log<Type>;
        using ClassArrayLogFcn = ArrayLogFcn<Type>;

        Log(const size_t dimension, ClassArrayLogFcn logFcn):
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
        ClassArrayLogFcn _arrayLogFcn;
};

template <typename Type>
class LogN: public Log<Type>
{
    public:

        using Class = LogN<Type>;
        using ClassArrayLogFcn = typename Log<Type>::ClassArrayLogFcn;

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
            using namespace std::placeholders;

            if(base <= 0)
            {
                throw Pothos::RangeException("Log base must be > 0");
            }

            _base = base;

            // We can't use switch because the base can be floating-point.
            if(_base == Type(2))  this->_arrayLogFcn = ClassArrayLogFcn(arrayLog2<Type>);
            if(_base == Type(10)) this->_arrayLogFcn = ClassArrayLogFcn(arrayLog10<Type>);
            else                  this->_arrayLogFcn = std::bind(arrayLogN<Type>, _1, _2, _base, _3);

            this->emitSignal("baseChanged");
        }

    protected:
        Type _base;
};

/***********************************************************************
 * Factory/registration
 **********************************************************************/

#define ifTypeDeclareLogFactory(Type, LogFcn) \
    if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(Type))) \
        return new Log<Type>(dtype.dimension(), &LogFcn<Type>);

#define LOGFACTORY(FactoryFcn, LogFcn) \
    static Pothos::Block* FactoryFcn(const Pothos::DType& dtype) \
    { \
        ifTypeDeclareLogFactory(double, LogFcn); \
        ifTypeDeclareLogFactory(float, LogFcn); \
        ifTypeDeclareLogFactory(int64_t, LogFcn); \
        ifTypeDeclareLogFactory(int32_t, LogFcn); \
        ifTypeDeclareLogFactory(int16_t, LogFcn); \
        ifTypeDeclareLogFactory(int8_t, LogFcn); \
        ifTypeDeclareLogFactory(uint64_t, LogFcn); \
        ifTypeDeclareLogFactory(uint32_t, LogFcn); \
        ifTypeDeclareLogFactory(uint16_t, LogFcn); \
        ifTypeDeclareLogFactory(uint8_t, LogFcn); \
        throw Pothos::InvalidArgumentException( #FactoryFcn "("+dtype.toString()+")", "unsupported type"); \
    }

LOGFACTORY(logFactory,   arrayLog)
LOGFACTORY(log2Factory,  arrayLog2)
LOGFACTORY(log10Factory, arrayLog10)

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