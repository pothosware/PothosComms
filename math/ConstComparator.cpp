// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>

//
// Templated function getters to be called on construction
//

template <typename Type>
using ConstComparatorFcn = void(*)(const Type*, Type, char*, size_t);

template <typename Type>
static inline ConstComparatorFcn<Type> getGreaterThanFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] > val) ? 1 : 0;
    };
}

template <typename Type>
static inline ConstComparatorFcn<Type> getLessThanFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] < val) ? 1 : 0;
    };
}

template <typename Type>
static inline ConstComparatorFcn<Type> getGreaterThanOrEqualFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] >= val) ? 1 : 0;
    };
}

template <typename Type>
static inline ConstComparatorFcn<Type> getLessThanOrEqualFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] <= val) ? 1 : 0;
    };
}

template <typename Type>
static inline ConstComparatorFcn<Type> getEqualToFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] == val) ? 1 : 0;
    };
}

template <typename Type>
static inline ConstComparatorFcn<Type> getNotEqualToFcn()
{
    return [](const Type* in, Type val, char* out, size_t num)
    {
        for(size_t i = 0; i < num; ++i) out[i] = (in[i] != val) ? 1 : 0;
    };
}

/*
 * Comparator functions
 */

#define COMPARATOR_FUNC(name,op) \
    template <typename Type> \
    void name(const Type *in0, const Type val, char *out, const size_t num) \
    { \
        for (size_t i = 0; i < num; i++) out[i] = in0[i] op val; \
    }

COMPARATOR_FUNC(greaterThan, >)
COMPARATOR_FUNC(lessThan, <)
COMPARATOR_FUNC(greaterOrEqualTo, >=)
COMPARATOR_FUNC(lessOrEqualTo, <=)
COMPARATOR_FUNC(equalTo, ==)
COMPARATOR_FUNC(notEqualTo, !=)

/***********************************************************************
 * |PothosDoc Const Comparator
 *
 * Perform a comparison between an input and given scalar value and output
 * the char 1 or 0.
 *
 * out[n] = (in0[n] $op value) ? 1 : 0;
 *
 * |category /Math
 * |keywords math logic comparator
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(float=1,int=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param comparator The comparison operation to perform
 * |default ">"
 * |option [>] ">"
 * |option [<] "<"
 * |option [>=] ">="
 * |option [<=] "<="
 * |option [==] "=="
 * |option [!=] "!="
 *
 * |param constant[Constant] The constant value to use in the operation.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |factory /comms/const_comparator(dtype,comparator)
 * |setter setConstant(constant)
 **********************************************************************/
template <typename Type>
class ConstComparator : public Pothos::Block
{
public:
    ConstComparator(const size_t dimension, ConstComparatorFcn<Type> fcn):
        _constant(0),
        _fcn(fcn)
    {
        typedef ConstComparator<Type> ClassType;

        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, typeid(char));

        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, constant));
        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, setConstant));

        this->registerProbe("constant");
        this->registerSignal("constantChanged");
    }

    Type constant() const
    {
        return _constant;
    }

    void setConstant(Type constant)
    {
        _constant = constant;
        this->emitSignal("constantChanged");
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
        char *out = outPort->buffer();

        //perform operation
        _fcn(in, _constant, out, elems*outPort->dtype().dimension());

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    Type _constant;
    ConstComparatorFcn<Type> _fcn;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *constComparatorFactory(const Pothos::DType &dtype, const std::string &operation)
{
    #define ifTypeDeclareFactory__(type, opKey, opFcn) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type)) and operation == opKey) \
            return new ConstComparator<type>(dtype.dimension(), opFcn<type>());
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory__(type, ">", getGreaterThanFcn) \
        ifTypeDeclareFactory__(type, "<", getLessThanFcn) \
        ifTypeDeclareFactory__(type, ">=", getGreaterThanOrEqualFcn) \
        ifTypeDeclareFactory__(type, "<=", getLessThanOrEqualFcn) \
        ifTypeDeclareFactory__(type, "==", getEqualToFcn) \
        ifTypeDeclareFactory__(type, "!=", getNotEqualToFcn)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    ifTypeDeclareFactory(uint64_t);
    ifTypeDeclareFactory(uint32_t);
    ifTypeDeclareFactory(uint16_t);
    ifTypeDeclareFactory(uint8_t);
    throw Pothos::InvalidArgumentException("Comparator("+dtype.toString()+", "+operation+")", "unsupported args");
}

static Pothos::BlockRegistry registerComparator(
    "/comms/const_comparator", &constComparatorFactory);

