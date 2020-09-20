// Copyright (c) 2014-2016 Tony Kirke
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <complex>
#include <algorithm> //min/max

//
// Implementation getters to be called on class construction
//

template <typename Type>
using ComparatorFcn = void(*)(const Type*, const Type*, char*, const size_t);

#ifdef POTHOS_XSIMD

// No (u)int16 support due to XSIMD limitation
template <typename Type>
struct IsSIMDComparatorSupported : std::integral_constant<bool,
    !std::is_same<Type, std::int16_t>::value &&
    !std::is_same<Type, std::uint16_t>::value> {};

template <typename Type>
using EnableForSIMDFcn = typename std::enable_if<IsSIMDComparatorSupported<Type>::value, ComparatorFcn<Type>>::type;

template <typename Type>
using EnableForDefaultFcn = typename std::enable_if<!IsSIMDComparatorSupported<Type>::value, ComparatorFcn<Type>>::type;

template <typename Type>
static inline EnableForSIMDFcn<Type> getGreaterThanFcn()
{
    return PothosCommsSIMD::greaterThanDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLessThanFcn()
{
    return PothosCommsSIMD::lessThanDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getGreaterOrEqualFcn()
{
    return PothosCommsSIMD::greaterThanOrEqualDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getLessOrEqualFcn()
{
    return PothosCommsSIMD::lessThanOrEqualDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getEqualToFcn()
{
    return PothosCommsSIMD::equalToDispatch<Type>();
}

template <typename Type>
static inline EnableForSIMDFcn<Type> getNotEqualToFcn()
{
    return PothosCommsSIMD::notEqualToDispatch<Type>();
}

#else

template <typename Type>
using EnableForDefaultFcn = ComparatorFcn<Type>;

#endif

template <typename Type>
static inline EnableForDefaultFcn<Type> getGreaterThanFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] > in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLessThanFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] < in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getGreaterOrEqualFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] >= in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getLessOrEqualFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] <= in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getEqualToFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] == in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline EnableForDefaultFcn<Type> getNotEqualToFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] != in1[i]) ? 1 : 0;
    };
}

/***********************************************************************
 * |PothosDoc Comparator
 *
 * Perform a comparision between 2 inputs and outputs a char value of 1 or 0
 *
 * out[n] = (in0[n] $op in1[n]) ? 1 : 0;
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
 * |factory /comms/comparator(dtype,comparator)
 **********************************************************************/
template <typename Type>
class Comparator : public Pothos::Block
{
public:
  Comparator(const size_t dimension, ComparatorFcn<Type> fcn): _fcn(fcn)
  {
    typedef Comparator<Type> ClassType;
    this->setupInput(0, Pothos::DType(typeid(Type), dimension));
    this->setupInput(1, Pothos::DType(typeid(Type), dimension));
    this->setupOutput(0, typeid(char));
  }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //get pointers to in and out buffer
        auto inPort0 = this->input(0);
        auto inPort1 = this->input(1);
        auto outPort = this->output(0);
        const Type *in0 = inPort0->buffer();
        const Type *in1 = inPort1->buffer();
        char *out = outPort->buffer();

        //perform operation
        _fcn(in0, in1, out, elems*outPort->dtype().dimension());

        //produce and consume on 0th ports
        inPort0->consume(elems);
        inPort1->consume(elems);
        outPort->produce(elems);
    }

private:
    ComparatorFcn<Type> _fcn;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *comparatorFactory(const Pothos::DType &dtype, const std::string &operation)
{
    #define ifTypeDeclareFactory__(type, opKey, opFcn) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type)) and operation == opKey) \
            return new Comparator<type>(dtype.dimension(), opFcn<type>());
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory__(type, ">", getGreaterThanFcn) \
        ifTypeDeclareFactory__(type, "<", getLessThanFcn) \
        ifTypeDeclareFactory__(type, ">=", getGreaterOrEqualFcn) \
        ifTypeDeclareFactory__(type, "<=", getLessOrEqualFcn) \
        ifTypeDeclareFactory__(type, "==", getEqualToFcn) \
        ifTypeDeclareFactory__(type, "!=", getNotEqualToFcn)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("Comparator("+dtype.toString()+", "+operation+")", "unsupported args");
}

static Pothos::BlockRegistry registerComparator(
    "/comms/comparator", &comparatorFactory);

