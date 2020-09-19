// Copyright (c) 2014-2016 Tony Kirke
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm> //min/max

//
// Implementation getters to be called on class construction
//

template <typename Type>
using ComparatorFcn = void(*)(const Type*, const Type*, char*, const size_t);

template <typename Type>
static inline ComparatorFcn<Type> getGreaterThanFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] > in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline ComparatorFcn<Type> getLessThanFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] < in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline ComparatorFcn<Type> getGreaterOrEqualFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] >= in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline ComparatorFcn<Type> getLessOrEqualFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] <= in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline ComparatorFcn<Type> getEqualToFcn()
{
    return [](const Type* in0, const Type* in1, char* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = (in0[i] == in1[i]) ? 1 : 0;
    };
}

template <typename Type>
static inline ComparatorFcn<Type> getNotEqualToFcn()
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

