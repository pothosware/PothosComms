// Copyright (c) 2014-2016 Josh Blum
//                    2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <cstdint>
#include <iostream>
#include <complex>
#include <vector>
#include <cstring> //memset
#include <algorithm> //min/max

//
// Implementation getters to be called on class construction
//

template <typename Type>
using ArithFcn = void(*)(const Type*, const Type*, Type*, const size_t);

template <typename Type>
static inline ArithFcn<Type> getAddFcn()
{
    return [](const Type* in0, const Type* in1, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = in0[i] + in1[i];
    };
}

template <typename Type>
static inline ArithFcn<Type> getSubFcn()
{
    return [](const Type* in0, const Type* in1, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = in0[i] - in1[i];
    };
}

template <typename Type>
static inline ArithFcn<Type> getMulFcn()
{
    return [](const Type* in0, const Type* in1, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = in0[i] * in1[i];
    };
}

template <typename Type>
static inline ArithFcn<Type> getDivFcn()
{
    return [](const Type* in0, const Type* in1, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = in0[i] / in1[i];
    };
}

/***********************************************************************
 * |PothosDoc Arithmetic
 *
 * Perform arithmetic operations on elements across multiple input ports to produce a stream of outputs.
 *
 * out[n] = in0[n] $op in1[n] $op ... $op in_last[n]
 *
 * |category /Math
 * |keywords math arithmetic add subtract multiply divide
 * |alias /blocks/arithmetic
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float32"
 * |preview disable
 *
 * |param operation The mathematical operation to perform
 * |default "ADD"
 * |option [Add] "ADD"
 * |option [Subtract] "SUB"
 * |option [Multiply] "MUL"
 * |option [Divide] "DIV"
 *
 * |param numInputs[Num Inputs] The number of input ports.
 * |default 2
 * |widget SpinBox(minimum=2)
 * |preview disable
 *
 * |param preload The number of elements to preload into each input.
 * The value is an array of integers where each element represents
 * the number of elements to preload the port with.
 * |default []
 * |widget ComboBox(editable=true)
 * |option [Ignored] \[\]
 * |preview disable
 *
 * |factory /comms/arithmetic(dtype, operation)
 * |initializer setNumInputs(numInputs)
 * |initializer setPreload(preload)
 **********************************************************************/
template <typename Type>
class Arithmetic : public Pothos::Block
{
public:
    Arithmetic(const size_t dimension, ArithFcn<Type> fcn):
        _numInlineBuffers(0),
        _fcn(fcn)
    {
        typedef Arithmetic<Type> ClassType;
        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, setNumInputs));
        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, setPreload));
        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, preload));
        this->registerCall(this, POTHOS_FCN_TUPLE(ClassType, getNumInlineBuffers));
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension), this->uid()); //unique domain because of inline buffer forwarding

        //read before write optimization
        this->output(0)->setReadBeforeWrite(this->input(0));
    }

    void setNumInputs(const size_t numInputs)
    {
        if (numInputs < 2) throw Pothos::RangeException("Arithmetic::setNumInputs("+std::to_string(numInputs)+")", "require inputs >= 2");
        for (size_t i = this->inputs().size(); i < numInputs; i++)
        {
            this->setupInput(i, this->input(0)->dtype());
        }
    }

    void setPreload(const std::vector<size_t> &preload)
    {
        this->setNumInputs(std::max<size_t>(2, preload.size()));
        _preload = preload;
    }

    const std::vector<size_t> &preload(void) const
    {
        return _preload;
    }

    void activate(void)
    {
        for (size_t i = 0; i < _preload.size(); i++)
        {
            auto bytes = _preload[i]*this->input(i)->dtype().size();
            if (bytes == 0) continue;
            Pothos::BufferChunk buffer(bytes);
            std::memset(buffer.as<void *>(), 0, buffer.length);
            this->input(i)->clear();
            this->input(i)->pushBuffer(buffer);
        }
    }

    void work(void)
    {
        //number of elements to work with
        auto elems = this->workInfo().minElements;
        if (elems == 0) return;

        //access to input ports and output port
        const std::vector<Pothos::InputPort *> &inputs = this->inputs();
        Pothos::OutputPort *output = this->output(0);

        //establish pointers to buffers
        Type *out = output->buffer();
        const Type *in0 = inputs[0]->buffer();
        if (out == in0) _numInlineBuffers++; //track buffer inlining

        //loop through available ports
        for (size_t i = 1; i < inputs.size(); i++)
        {
            const Type *inX = inputs[i]->buffer();
            _fcn(in0, inX, out, elems*output->dtype().dimension());
            in0 = out; //operate on output array next loop
            inputs[i]->consume(elems); //consume on ith input port
        }

        //produce and consume on 0th ports
        inputs[0]->consume(elems);
        output->produce(elems);
    }

    void propagateLabels(const Pothos::InputPort *port)
    {
        //this is a feedback port, don't propagate labels from it
        if (_preload.size() > size_t(port->index()) and _preload[port->index()] > 0) return;

        //otherwise implement the regular label propagation
        Pothos::Block::propagateLabels(port);
    }

    size_t getNumInlineBuffers(void) const
    {
        return _numInlineBuffers;
    }

private:
    size_t _numInlineBuffers;
    std::vector<size_t> _preload;

    ArithFcn<Type> _fcn;
};

/***********************************************************************
 * templated arithmetic vector operators
 **********************************************************************/
template <typename Type>
void addArray(const Type *in0, const Type *in1, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++) out[i] = in0[i] + in1[i];
}

template <typename Type>
void subArray(const Type *in0, const Type *in1, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++) out[i] = in0[i] - in1[i];
}

template <typename Type>
void mulArray(const Type *in0, const Type *in1, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++) out[i] = in0[i] * in1[i];
}

template <typename Type>
void divArray(const Type *in0, const Type *in1, Type *out, const size_t num)
{
    for (size_t i = 0; i < num; i++) out[i] = in0[i] / in1[i];
}

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *arithmeticFactory(const Pothos::DType &dtype, const std::string &operation)
{
    #define ifTypeDeclareFactory__(type, opKey, opFcn) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type)) and operation == opKey) \
            return new Arithmetic<type>(dtype.dimension(), opFcn<type>());
    #define ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory__(type, "ADD", getAddFcn) \
        ifTypeDeclareFactory__(type, "SUB", getSubFcn) \
        ifTypeDeclareFactory__(type, "MUL", getMulFcn) \
        ifTypeDeclareFactory__(type, "DIV", getDivFcn)
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory_(std::complex<type>)
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    ifTypeDeclareFactory(uint64_t);
    ifTypeDeclareFactory(uint32_t);
    ifTypeDeclareFactory(uint16_t);
    ifTypeDeclareFactory(uint8_t);
    ifTypeDeclareFactory(int64_t);
    ifTypeDeclareFactory(int32_t);
    ifTypeDeclareFactory(int16_t);
    ifTypeDeclareFactory(int8_t);
    throw Pothos::InvalidArgumentException("arithmeticFactory("+dtype.toString()+", "+operation+")", "unsupported args");
}

static Pothos::BlockRegistry registerArithmetic(
    "/comms/arithmetic", &arithmeticFactory);

static Pothos::BlockRegistry registerArithmeticOldPath(
    "/blocks/arithmetic", &arithmeticFactory);
