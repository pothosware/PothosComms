// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <complex>
#include <cstdint>
#include <string>

/***********************************************************************
 * |PothosDoc Const Arithmetic
 *
 * Perform arithmetic operations on each element, using a user-given
 * constant as an operand.
 *
 * |category /Math
 * |keywords math arithmetic add subtract multiply divide
 *
 * |param dtype[Data Type] The data type used in the arithmetic.
 * |widget DTypeChooser(int=1,uint1=,float=1,cint=1,cuint=1,cfloat=1,dim=1)
 * |default "float32"
 * |preview disable
 *
 * |param operation The mathematical operation to perform.
 * |widget ComboBox(editable=false)
 * |default "ADD"
 * |option [Add] "ADD"
 * |option [Subtract] "SUB"
 * |option [Multiply] "MUL"
 * |option [Divide] "DIV"
 * |preview enable
 *
 * |param constant[Constant] The constant value to use in the operation.
 * |widget LineEdit()
 * |default 0
 * |preview enable
 *
 * |param constOperandPosition[Const Position] The position of the constant operand.
 * |widget ComboBox(editable=false)
 * |option 0
 * |option 1
 * |default 0
 * |preview enable
 *
 * |factory /comms/const_arithmetic(dtype,operation,constant)
 * |setter setConstant(constant)
 * |setter setConstOperandPosition(constOperandPosition)
 **********************************************************************/
template <typename T>
class ConstArithmetic: public Pothos::Block
{
public:
    using OperatorFcn = T(*)(const T&, const T&);
    using Class = ConstArithmetic<T>;

    ConstArithmetic(
        OperatorFcn operatorFcn,
        const T& constant,
        size_t dimension
    ): Pothos::Block(),
       _constant(0),
       _position(0),
       _operator(operatorFcn)
    {
        const Pothos::DType dtype(typeid(T), dimension);
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, constant));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstant));

        this->registerCall(this, POTHOS_FCN_TUPLE(Class, constOperandPosition));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setConstOperandPosition));

        this->registerProbe("constant");
        this->registerSignal("constantChanged");

        // Call setter to emit signal
        this->setConstant(constant);
    }

    T constant() const
    {
        return _constant;
    }

    void setConstant(const T& constant)
    {
        _constant = constant;

        this->updateCallable();
        this->emitSignal("constantChanged", constant);
    }

    size_t constOperandPosition() const
    {
        return _position;
    }

    void setConstOperandPosition(size_t position)
    {
        if((0 == position) || (1 == position))
        {
            _position = position;
            this->updateCallable();
        }
        else
        {
            throw Pothos::InvalidArgumentException(
                      "Invalid position",
                      std::to_string(position));
        }
    }

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if(0 == elems)
        {
            return;
        }

        auto* input = this->input(0);
        auto* output = this->output(0);

        const T* buffIn = input->buffer();
        T* buffOut = output->buffer();

        for(size_t elem = 0; elem < elems; ++elem)
        {
            buffOut[elem] = _operator.call(buffIn[elem]).template extract<T>();
        }

        input->consume(elems);
        output->produce(elems);
    }

private:
    T _constant;
    size_t _position;
    Pothos::Callable _operator;

    void updateCallable()
    {
        _operator.unbind(0);
        _operator.unbind(1);
        _operator.bind(_constant, _position);
    }
};

//
// Registration
//

static Pothos::Block* makeConstArithmetic(
    const Pothos::DType& dtype,
    const std::string& operation,
    const Pothos::Object& constant)
{
    #define ifTypeDeclareFactory__(type, opKey, func) \
        if((Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) && (opKey == operation)) \
            return new ConstArithmetic<type>(func, constant.convert<type>(), dtype.dimension());
    #define ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory__(type, "ADD", [](const type& a, const type& b)->type {return a+b;}) \
        ifTypeDeclareFactory__(type, "SUB", [](const type& a, const type& b)->type {return a-b;}) \
        ifTypeDeclareFactory__(type, "MUL", [](const type& a, const type& b)->type {return a*b;}) \
        ifTypeDeclareFactory__(type, "DIV", [](const type& a, const type& b)->type {return a/b;})
    #define ifTypeDeclareFactory(type) \
        ifTypeDeclareFactory_(type) \
        ifTypeDeclareFactory_(std::complex<type>)

    ifTypeDeclareFactory(std::int8_t)
    ifTypeDeclareFactory(std::int16_t)
    ifTypeDeclareFactory(std::int32_t)
    ifTypeDeclareFactory(std::int64_t)
    ifTypeDeclareFactory(std::uint8_t)
    ifTypeDeclareFactory(std::uint16_t)
    ifTypeDeclareFactory(std::uint32_t)
    ifTypeDeclareFactory(std::uint64_t)
    ifTypeDeclareFactory(float)
    ifTypeDeclareFactory(double)

    throw Pothos::InvalidArgumentException(
              "makeConstArithmetic("+dtype.toString()+", operation="+operation+")",
              "unsupported args");
}

static Pothos::BlockRegistry registerConstArithmetic(
    "/comms/const_arithmetic",
    Pothos::Callable(&makeConstArithmetic));
