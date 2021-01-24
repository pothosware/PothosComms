// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#ifdef POTHOS_XSIMD
#include "SIMD/MathBlocks_SIMD.hpp"
#endif

#include <Pothos/Callable.hpp>
#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

//
// Utility code
//

template <typename T, typename Ret>
using EnableIfFloatingPoint = typename std::enable_if<std::is_floating_point<T>::value, Ret>::type;

template <typename T, typename Ret>
using EnableIfNotFloatingPoint = typename std::enable_if<!std::is_floating_point<T>::value, Ret>::type;

template <typename T, typename Ret>
using EnableIfSigned = typename std::enable_if<std::is_signed<T>::value, Ret>::type;

template <typename T, typename Ret>
using EnableIfUnsigned = typename std::enable_if<std::is_unsigned<T>::value, Ret>::type;

/*
 * "For odd numbered roots (e.g. cubic) and negative numbers, the result
 * of the root is well defined and negative, but just using pow(value, 1.0/n)
 * won't work (you get back "NaN" - not a number)."
 *
 * https://stackoverflow.com/a/28255068
 */
template <typename T>
static inline EnableIfSigned<T, T> getAvoidNaNFactor(T value)
{
    return T((value < 0) ? -1 : 1);
}

template <typename T>
static inline EnableIfUnsigned<T, T> getAvoidNaNFactor(T)
{
    return T(1);
}

//
// Implementation getters, called on class construction.
//

// Use a std::function instead of a function pointer because
// GetNthRoot's lambda needs to capture a parameter.
template <typename T>
using RootFcn = std::function<void(const T*, T*, size_t)>;

#ifdef POTHOS_XSIMD

template <typename T>
using EnableForSIMDFcn = typename std::enable_if<std::is_floating_point<T>::value, RootFcn<T>>::type;

template <typename T>
using EnableForDefaultFcn = typename std::enable_if<!std::is_floating_point<T>::value, RootFcn<T>>::type;

template <typename T>
static inline EnableForSIMDFcn<T> getSqrtFcn()
{
    return PothosCommsSIMD::sqrtDispatch<T>();
}

template <typename T>
static inline EnableForSIMDFcn<T> getCbrtFcn()
{
    return PothosCommsSIMD::cbrtDispatch<T>();
}

#else
template <typename T>
using EnableForDefaultFcn = RootFcn<T>;
#endif

template <typename T>
static inline EnableForDefaultFcn<T> getSqrtFcn()
{
    return [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(std::sqrt(in[i]));
    };
}

template <typename T>
static inline EnableForDefaultFcn<T> getCbrtFcn()
{
    return [](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i) out[i] = T(std::cbrt(in[i]));
    };
}

//
// Nth root: SFINAE is our friend
//

template <typename T>
static inline EnableIfFloatingPoint<T, RootFcn<T>> _getNthRootFcnOdd(T root)
{
    return [root](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i)
        {
            T f = getAvoidNaNFactor(in[i]);
            out[i] = T(std::pow(in[i] * f, 1.0 / root) * f);
        }
    };
}

template <typename T>
static inline EnableIfFloatingPoint<T, RootFcn<T>> _getNthRootFcnEven(T root)
{
    return [root](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i)
        {
            out[i] = T(std::pow(in[i], 1.0 / root));
        }
    };
}

template <typename T>
static inline EnableIfNotFloatingPoint<T, RootFcn<T>> _getNthRootFcnOdd(T root)
{
    return [root](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i)
        {
            T f = getAvoidNaNFactor(in[i]);
            out[i] = T(std::round(std::pow(in[i] * f, 1.0 / root) * f));
        }
    };
}

template <typename T>
static inline EnableIfNotFloatingPoint<T, RootFcn<T>> _getNthRootFcnEven(T root)
{
    return [root](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i)
        {
            out[i] = T(std::round(std::pow(in[i], 1.0 / root)));
        }
    };
}

template <typename T>
static inline EnableIfSigned<T, RootFcn<T>> getNthRootFcn(T root)
{
    return (std::fmod(root, T(2.0)) == 1) ? _getNthRootFcnOdd(root) : _getNthRootFcnEven(root);
}

template <typename T>
static inline EnableIfUnsigned<T, RootFcn<T>> getNthRootFcn(T root)
{
    return [root](const T* in, T* out, size_t num)
    {
        for (size_t i = 0; i < num; ++i)
        {
            out[i] = T(std::round(std::pow(in[i], 1.0 / root)));
        }
    };
}

/***********************************************************************
 * Implementation
 **********************************************************************/

template <typename Type>
class Root : public Pothos::Block
{
public:
    using Class = Root<Type>;
    using ClassRootFcn = RootFcn<Type>;

    Root(const size_t dimension, ClassRootFcn fcn) :
        _fcn(fcn)
    {
        this->setupInput(0, Pothos::DType(typeid(Type), dimension));
        this->setupOutput(0, Pothos::DType(typeid(Type), dimension));
    }

    virtual ~Root() = default;

    void work() override
    {
        const auto elems = this->workInfo().minElements;
        if (0 == elems) return;

        auto input = this->input(0);
        auto output = this->output(0);

        _fcn(input->buffer(), output->buffer(), elems*input->dtype().dimension());

        input->consume(elems);
        output->produce(elems);
    }

protected:
    ClassRootFcn _fcn;
};

template <typename Type>
class NthRoot : public Root<Type>
{
public:

    using Class = NthRoot<Type>;
    using ClassArrayRootFcn = typename Root<Type>::ClassRootFcn;

    NthRoot(const size_t dimension, Type root) : Root<Type>(dimension, nullptr)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, root));
        this->registerCall(this, POTHOS_FCN_TUPLE(Class, setRoot));
        this->registerProbe("root");
        this->registerSignal("rootChanged");

        this->setRoot(root);
    }

    virtual ~NthRoot() = default;

    Type root() const
    {
        return _root;
    }

    void setRoot(Type root)
    {
        _root = root;

        // We can't use switch because the root can be floating-point.
        if (_root == Type(2)) this->_fcn = getSqrtFcn<Type>();
        if (_root == Type(3)) this->_fcn = getCbrtFcn<Type>();
        else                  this->_fcn = getNthRootFcn<Type>(_root);

        this->emitSignal("rootChanged");
    }

protected:
    Type _root;
};

//
// Registration
//

static Pothos::Block* makeSqrt(const Pothos::DType& dtype)
{
    #define ifTypeReturnSqrt(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Root<T>(dtype.dimension(), getSqrtFcn<T>());

    ifTypeReturnSqrt(std::int8_t)
    ifTypeReturnSqrt(std::int16_t)
    ifTypeReturnSqrt(std::int32_t)
    ifTypeReturnSqrt(std::int64_t)
    ifTypeReturnSqrt(std::uint8_t)
    ifTypeReturnSqrt(std::uint16_t)
    ifTypeReturnSqrt(std::uint32_t)
    ifTypeReturnSqrt(std::uint64_t)
    ifTypeReturnSqrt(float)
    ifTypeReturnSqrt(double)

    throw Pothos::InvalidArgumentException("makeSqrt: unsupported type: " + dtype.toString());
}

static Pothos::Block* makeCbrt(const Pothos::DType& dtype)
{
    #define ifTypeReturnCbrt(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new Root<T>(dtype.dimension(), getCbrtFcn<T>());

    ifTypeReturnCbrt(std::int8_t)
    ifTypeReturnCbrt(std::int16_t)
    ifTypeReturnCbrt(std::int32_t)
    ifTypeReturnCbrt(std::int64_t)
    ifTypeReturnCbrt(std::uint8_t)
    ifTypeReturnCbrt(std::uint16_t)
    ifTypeReturnCbrt(std::uint32_t)
    ifTypeReturnCbrt(std::uint64_t)
    ifTypeReturnCbrt(float)
    ifTypeReturnCbrt(double)

    throw Pothos::InvalidArgumentException("makeCbrt: unsupported type: " + dtype.toString());
}

static Pothos::Block* makeNthRoot(const Pothos::DType& dtype, const Pothos::Object& root)
{
    #define ifTypeReturnNthRoot(T) \
        if(Pothos::DType::fromDType(dtype, 1) == Pothos::DType(typeid(T))) \
            return new NthRoot<T>(dtype.dimension(), root.convert<T>());

    ifTypeReturnNthRoot(std::int8_t)
    ifTypeReturnNthRoot(std::int16_t)
    ifTypeReturnNthRoot(std::int32_t)
    ifTypeReturnNthRoot(std::int64_t)
    ifTypeReturnNthRoot(std::uint8_t)
    ifTypeReturnNthRoot(std::uint16_t)
    ifTypeReturnNthRoot(std::uint32_t)
    ifTypeReturnNthRoot(std::uint64_t)
    ifTypeReturnNthRoot(float)
    ifTypeReturnNthRoot(double)

    throw Pothos::InvalidArgumentException("makeNthRoot: unsupported type: " + dtype.toString());
}

/***********************************************************************
 * |PothosDoc Square Root
 *
 * Calculate the square root of each input element.
 *
 * out[n] = sqrt(in[n])
 *
 * |category /Math
 * |keywords sqrt
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/sqrt(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerSqrt(
    "/comms/sqrt",
    &makeSqrt);

/***********************************************************************
 * |PothosDoc Cube Root
 *
 * Calculate the cube root of each input element.
 *
 * out[n] = cbrt(in[n])
 *
 * |category /Math
 * |keywords cbrt
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |factory /comms/cbrt(dtype)
 **********************************************************************/
static Pothos::BlockRegistry registerCbrt(
    "/comms/cbrt",
    &makeCbrt);

/***********************************************************************
 * |PothosDoc Nth Root
 *
 * Calculate the Nth root of each input element, for a given N.
 * Has optimizations for roots <b>2</b> and <b>3</b>.
 *
 * out[n] = root(in[n], N)
 *
 * |category /Math
 * |setter setRoot(root)
 *
 * |param dtype[Data Type] The data type.
 * |widget DTypeChooser(float=1,int=1,uint=1,dim=1)
 * |default "float64"
 * |preview disable
 *
 * |param root[Root] The root for calculation.
 * |widget SpinBox()
 * |default 1
 * |preview enable
 *
 * |factory /comms/nth_root(dtype,root)
 * |setter setRoot(root)
 **********************************************************************/
static Pothos::BlockRegistry registerNthRoot(
    "/comms/nth_root",
    &makeNthRoot);
