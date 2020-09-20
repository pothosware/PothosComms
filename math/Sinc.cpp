// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>

#include <cmath>

//
// Implementation getters to be called on class construction
//

template <typename Type>
using SincFcn = void(*)(const Type*, Type*, const size_t);

template <typename Type>
static inline SincFcn<Type> getSincFcn()
{
    return [](const Type* in, Type* out, const size_t num)
    {
        for (size_t i = 0; i < num; i++)
        {
            #define ZERO(x) (std::abs(x) < 1e-6)
            out[i] = ZERO(in[i]) ? 1 : (std::sin(in[i]) / in[i]);
        }
    };
}

/***********************************************************************
 * |PothosDoc Sinc
 *
 * Perform the sinc function on all inputs, defined as:
 *
 * <ul>
 * <li>(in[n] == 0) <b>-></b> (out[n] = 1)</li>
 * <li>(in[n] != 0) <b>-></b> (out[n] = sin(in[n]) / in[n])</li>
 * </ul>
 *
 * From the NumPy documentation:
 *
 * <b>sinc(0)</b> is the limit value 1.
 *
 * The name sinc is short for "sine cardinal" or "sinus cardinalis".
 *
 * The sinc function is used in various signal processing applications,
 * including in anti-aliasing, in the construction of a Lanczos resampling
 * filter, and in interpolation.
 *
 * |category /Math
 * |keywords math sin
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/sinc(dtype)
 **********************************************************************/
template <typename Type>
class Sinc : public Pothos::Block
{
public:
    using Class = Sinc<Type>;

    Sinc(const size_t dimension): _fcn(getSincFcn<Type>())
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
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
        Type *out = outPort->buffer();

        const size_t N = elems*inPort->dtype().dimension();
        _fcn(in, out, N);

        //produce and consume on 0th ports
        inPort->consume(elems);
        outPort->produce(elems);
    }

private:
    SincFcn<Type> _fcn;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block* sincFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(type))) \
            return new Sinc<type>(dtype.dimension());
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    throw Pothos::InvalidArgumentException("sincFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerSinc(
    "/comms/sinc",
    Pothos::Callable(&sincFactory));
