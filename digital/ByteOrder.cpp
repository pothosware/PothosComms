// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "ByteOrder.hpp"

#include <Pothos/Exception.hpp>
#include <Pothos/Framework.hpp>

#include <Poco/Platform.h>

#include <algorithm>
#include <complex>
#include <cstring>
#include <unordered_map>

//
// Templated implementation getters, to be called on class construction
//

template <typename T>
using ByteOrderFcn = void(*)(const T*, T*, size_t);

template <typename T>
static inline ByteOrderFcn<T> getFlipBytesFcn()
{
    return byteswapBuffer<T>;
}

template <typename T>
static inline ByteOrderFcn<T> getBigEndianFcn()
{
#if defined(POCO_ARCH_BIGENDIAN)
    return [](const T* in, T* out, size_t numElements)
    {
        std::memcpy(out, in, (numElements * sizeof(T)));
    };
#else
    return getFlipBytesFcn<T>();
#endif
}

template <typename T>
static inline ByteOrderFcn<T> getLittleEndianFcn()
{
#if defined(POCO_ARCH_BIGENDIAN)
    return getFlipBytesFcn<T>();
#else
    return [](const T* in, T* out, size_t numElements)
    {
        std::memcpy(out, in, (numElements * sizeof(T)));
    };
#endif
}

template <typename T>
static inline ByteOrderFcn<T> getToNetworkFcn()
{
    return getBigEndianFcn<T>();
}

template <typename T>
static inline ByteOrderFcn<T> getFromNetworkFcn()
{
    return getBigEndianFcn<T>();
}

//
// Class implementation
//

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

template <typename T>
static ByteOrderFcn<T> getByteOrderFcn(const std::string& byteOrder)
{
    using ByteOrderFcnMap = std::unordered_map<std::string, ByteOrderFcn<T>>;
    static const ByteOrderFcnMap FcnMap =
    {
        {"Swap Order",      getFlipBytesFcn<T>()},
        {"Big Endian",      getBigEndianFcn<T>()},
        {"Little Endian",   getLittleEndianFcn<T>()},
        {"Network to Host", getFromNetworkFcn<T>()},
        {"Host to Network", getToNetworkFcn<T>()}
    };

    auto iter = FcnMap.find(byteOrder);
    if (iter != FcnMap.end()) return iter->second;
    else
    {
        throw Pothos::InvalidArgumentException(
            "Invalid byte order",
            byteOrder);
    }
}

/***********************************************************************
 * |PothosDoc Byte Order
 *
 * Sets the byte ordering of all incoming packets and buffers.
 *
 * |category /Digital
 * |keywords bytes big little host network endian
 *
 * |param dtype[Data Type] The output data type produced by the mapper.
 * |widget DTypeChooser(int=1,uint=1,float=1,cint=1,cuint=1,cfloat=1,dim=1)
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
        _order(ByteOrderType::Swap),
        _fcn(getByteOrderFcn<T>("Swap Order"))
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
        _fcn = getByteOrderFcn<T>(order);
    }

    void msgWork(const Pothos::Packet &inPkt)
    {
        const auto numElements = inPkt.payload.length / sizeof(T);
        
        Pothos::Packet outPkt;
        auto outPort = this->output(0);
        outPkt.payload = outPort->getBuffer(numElements);

        _fcn(inPkt.payload, outPkt.payload, numElements);

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

        _fcn(inPort->buffer(), outPort->buffer(), numElements*inPort->dtype().dimension());

        inPort->consume(numElements);
        outPort->produce(numElements);
    }

private:
    ByteOrderType _order;
    ByteOrderFcn<T> _fcn;
};

static Pothos::Block* makeByteOrder(const Pothos::DType& dtype)
{
    #define IfTypeDeclareBlock(T) \
        if(Pothos::DType(typeid(T)) == Pothos::DType::fromDType(dtype, 1)) \
            return new ByteOrder<T>(dtype.dimension()); \
        if(Pothos::DType(typeid(std::complex<T>)) == Pothos::DType::fromDType(dtype, 1)) \
            return new ByteOrder<std::complex<T>>(dtype.dimension()); \

    IfTypeDeclareBlock(std::int16_t)
    IfTypeDeclareBlock(std::int32_t)
    IfTypeDeclareBlock(std::int64_t)
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
