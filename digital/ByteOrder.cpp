// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/ByteOrder.h>

#include <algorithm>
#include <complex>
#include <type_traits>
#include <unordered_map>

#if POCO_OS == POCO_OS_MAC_OS_X
#include <libkern/OSByteOrder.h>
#endif

//
// This type_traits SFINAE magic results in the correct functions being
// called for a given type.
//

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T>
struct BufferType
{
    using Type = T;
};

template <>
struct BufferType<float>
{
    using Type = Poco::UInt32;
};

template <>
struct BufferType<double>
{
    using Type = Poco::UInt64;
};

#if POCO_OS == POCO_OS_MAC_OS_X

// For some reason, despite specifically supporting OS X, Poco uses manual
// bit-shifting instead of OS X's optimized byteswap functions. Unfortunately,
// that means we have to implement enough that GENERATE_FUNCS() uses these
// functions for OS X.
#define GENERATE_BASE_FUNC(func)

    static inline Poco::UInt16 byteswap(Poco::UInt16 val)
    {
        return OSSwapInt16(val);
    }

    static inline Poco::UInt32 byteswap(Poco::UInt32 val)
    {
        return OSSwapInt32(val);
    }

    static inline Poco::UInt64 byteswap(Poco::UInt64 val)
    {
        return OSSwapInt64(val);
    }

    template <typename T>
    static inline typename std::enable_if<!IsComplex<T>::value && std::is_floating_point<T>::value, T>::type byteswap(T val)
    {
        const auto* pCastedVal = reinterpret_cast<const typename BufferType<double>::Type*>(&val);
        const auto ret = byteswap(*pCastedVal);
        const auto* pCastedRet = reinterpret_cast<const T*>(&ret);

        return *pCastedRet;
    }

    template <typename T>
    static inline T flipBytesBase(T val)
    {
        return byteswap(val);
    }

    template <typename T>
    static inline T toBigEndianBase(T val)
    {
#if defined(POCO_ARCH_BIGENDIAN)
        return val;
#else
        return byteswap(val);
#endif
    }

    template <typename T>
    static inline T toLittleEndianBase(T val)
    {
#if defined(POCO_ARCH_BIGENDIAN)
        return byteswap(val);
#else
        return val;
#endif
    }

    template <typename T>
    static inline T toNetworkBase(T val)
    {
#if defined(POCO_ARCH_BIGENDIAN)
        return val;
#else
        return byteswap(val);
#endif
    }

    template <typename T>
    static inline T fromNetworkBase(T val)
    {
#if defined(POCO_ARCH_BIGENDIAN)
        return val;
#else
        return byteswap(val);
#endif
    }

#else

#define GENERATE_BASE_FUNC(func) \
    template <typename T> \
    static inline T func ## Base(T val) \
    { \
        return Poco::ByteOrder::func(val); \
    }
#endif

#define GENERATE_FUNCS(func) \
    GENERATE_BASE_FUNC(func) \
 \
    template <typename T> \
    static inline typename std::enable_if<!IsComplex<T>::value && !std::is_floating_point<T>::value, T>::type func(T val) \
    { \
        return func ## Base(val); \
    } \
 \
    template <typename T> \
    static inline typename std::enable_if<!IsComplex<T>::value && std::is_floating_point<T>::value, T>::type func(T val) \
    { \
        static_assert(sizeof(T) == sizeof(typename BufferType<T>::Type), "type size mismatch"); \
        const auto* pCastedVal = reinterpret_cast<const typename BufferType<T>::Type*>(&val); \
        const auto ret = func ## Base(*pCastedVal); \
        const auto* pCastedRet = reinterpret_cast<const T*>(&ret); \
        return *pCastedRet; \
    } \
 \
    template <typename T> \
    static inline typename std::enable_if<IsComplex<T>::value, T>::type func(T val) \
    { \
        return T{func(val.real()), func(val.imag())}; \
    } \
 \
    template <typename T> \
    static inline void func ## Buffer(T* out, const T* in, size_t numElements) \
    { \
        for(size_t i = 0; i < numElements; ++i) out[i] = func(in[i]); \
    }

GENERATE_FUNCS(flipBytes)
GENERATE_FUNCS(toBigEndian)
GENERATE_FUNCS(toLittleEndian)
GENERATE_FUNCS(toNetwork)
GENERATE_FUNCS(fromNetwork)

enum class ByteOrderType
{
    Swap,
    Big,
    Little,
    Host,
    Network
};

using ByteOrderTypeMap = std::unordered_map<std::string, ByteOrderType>;
static const ByteOrderTypeMap TypeMap =
{
    {"Swap Order",      ByteOrderType::Swap},
    {"Big Endian",      ByteOrderType::Big},
    {"Little Endian",   ByteOrderType::Little},
    {"Network to Host", ByteOrderType::Host},
    {"Host to Network", ByteOrderType::Network}
};

/***********************************************************************
 * |PothosDoc Byte Order
 *
 * Sets the byte ordering of all incoming packets and buffers.
 *
 * |category /Digital
 * |keywords bytes big little host network endian
 *
 * |param dtype[Data Type] The output data type produced by the mapper.
 * |widget DTypeChooser(uint=1,cuint=1,dim=1)
 * |default "uint64"
 * |preview disable
 *
 * |param byteOrder[Byte Order] The byte ordering.
 * |widget ComboBox(editable=false)
 *
 * <ul>
 * <li><b>Swap Order:</b> Swap the byte-ordering regardless of the incoming order.
 * <li><b>Big Endian:</b> Most significant byte first. Does nothing on big-endian platforms.
 * <li><b>Little Endian:</b> Least significant type first. Does nothing on little-endian platforms.
 * <li><b>Network to Host:</b> Swaps from network byte order (big endian) to host order. Does nothing on big-endian platforms.
 * <li><b>Host to Network:</b> Swaps from host byte order to network order. Does nothing on big-endian platforms.
 * </ul>
 *
 * |option [Swap Order] "Swap Order"
 * |option [Big Endian] "Big Endian"
 * |option [Little Endian] "Little Endian"
 * |option [Network to Host] "Network to Host"
 * |option [Host to Network] "Host to Network"
 * |default "Swap Order"
 *
 * |factory /comms/byte_order(dtype)
 * |setter setByteOrder(byteOrder)
 **********************************************************************/
template <typename T>
class ByteOrder : public Pothos::Block
{
public:
    ByteOrder(size_t dimension):
        _order(ByteOrderType::Swap)
    {
        const Pothos::DType dtype(typeid(T), dimension);
        
        this->setupInput(0, dtype);
        this->setupOutput(0, dtype);
        this->registerCall(this, POTHOS_FCN_TUPLE(ByteOrder, setByteOrder));
        this->registerCall(this, POTHOS_FCN_TUPLE(ByteOrder, getByteOrder));
    }

    std::string getByteOrder(void) const
    {
        auto mapIter = std::find_if(
                           TypeMap.begin(),
                           TypeMap.end(),
                           [this](const ByteOrderTypeMap::value_type& mapPair)
                           {return (mapPair.second == _order);});
        if(TypeMap.end() == mapIter)
        {
            throw Pothos::AssertionViolationException(
                      "Couldn't find string representation of byte order enum",
                      std::to_string(static_cast<int>(_order)));
        }
        
        return mapIter->first;
    }

    void setByteOrder(const std::string& order)
    {
        auto mapIter = TypeMap.find(order);
        if(TypeMap.end() == mapIter)
        {
            throw Pothos::InvalidArgumentException(
                      "Invalid byte order",
                      order);
        }

        _order = mapIter->second;
    }

    void bufferWork(T* out, const T* in, size_t numElements)
    {
        switch(_order)
        {
            case ByteOrderType::Swap:
                flipBytesBuffer(out, in, numElements);
                break;

            case ByteOrderType::Big:
                toBigEndianBuffer(out, in, numElements);
                break;

            case ByteOrderType::Little:
                toLittleEndianBuffer(out, in, numElements);
                break;

            case ByteOrderType::Host:
                fromNetworkBuffer(out, in, numElements);
                break;

            case ByteOrderType::Network:
                toNetworkBuffer(out, in, numElements);
                break;

            default:
                throw Pothos::AssertionViolationException(
                          "Private enum field is set to an invalid value",
                          std::to_string(static_cast<int>(_order)));
        }
    }

    void msgWork(const Pothos::Packet &inPkt)
    {
        const auto numElements = inPkt.payload.length / sizeof(T);
        
        Pothos::Packet outPkt;
        auto outPort = this->output(0);
        outPkt.payload = outPort->getBuffer(numElements);

        bufferWork(
            outPkt.payload.template as<T*>(),
            inPkt.payload.as<const T*>(),
            numElements);

        // Copy labels.
        outPkt.labels = inPkt.labels;

        // Pass the message on with the new byte ordering.
        outPort->postMessage(std::move(outPkt));
    }

    void work(void)
    {
        auto inPort = this->input(0);
        auto outPort = this->output(0);

        // Process an incoming packet, or pass along the given message.
        if(inPort->hasMessage())
        {
            auto msg = inPort->popMessage();
            if (msg.type() == typeid(Pothos::Packet))
                this->msgWork(msg.template extract<Pothos::Packet>());
            else outPort->postMessage(std::move(msg));
            return;
        }

        const auto numElements = std::min(inPort->elements(), outPort->elements());
        if(0 == numElements)
        {
            return;
        }

        bufferWork(
            outPort->buffer().template as<T*>(),
            inPort->buffer().template as<const T*>(),
            numElements*inPort->dtype().dimension());

        inPort->consume(numElements);
        outPort->produce(numElements);
    }

private:
    ByteOrderType _order;
};

static Pothos::Block* makeByteOrder(const Pothos::DType& dtype)
{
    #define IfTypeDeclareBlock(T) \
        if(Pothos::DType(typeid(T)) == Pothos::DType::fromDType(dtype, 1)) \
            return new ByteOrder<T>(dtype.dimension()); \
        if(Pothos::DType(typeid(std::complex<T>)) == Pothos::DType::fromDType(dtype, 1)) \
            return new ByteOrder<std::complex<T>>(dtype.dimension()); \

    IfTypeDeclareBlock(std::uint16_t)
    IfTypeDeclareBlock(std::uint32_t)
    IfTypeDeclareBlock(std::uint64_t)
    IfTypeDeclareBlock(float)
    IfTypeDeclareBlock(double)

    throw Pothos::InvalidArgumentException(
              "Unsupported or invalid type",
              dtype.name());
}

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::BlockRegistry registerByteOrder(
    "/comms/byte_order", &makeByteOrder);
