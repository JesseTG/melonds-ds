#include "mp.hpp"
#include "environment.hpp"
#include <ctime>
#include <libretro.h>
#include <retro_assert.h>
#include <retro_endianness.h>
using namespace MelonDsDs;

constexpr long RECV_TIMEOUT_MS = 25;

uint64_t swapToNetwork(uint64_t n) {
    return swap_if_little64(n);
}

Packet Packet::parsePk(const void *buf, uint64_t len) {
    // Necessary because arithmetic on void* is forbidden
    const char *indexableBuf = (const char *)buf;
    const char *data = indexableBuf + HeaderSize;
    retro_assert(len >= HeaderSize);
    size_t dataLen = len - HeaderSize;
    uint64_t timestamp = swapToNetwork(*(const uint64_t*)(indexableBuf));
    uint8_t aid = *(const uint8_t*)(indexableBuf + 8);
    uint8_t type = *(const uint8_t*)(indexableBuf + 9);
    // type 2 means cmd frame
    // type 1 means reply frame
    // type 0 means anything else
    retro_assert(type == 2 || type == 1 || type == 0);
    Packet::Type pkType;
    switch (type) {
        case 0:
            pkType = Other;
            break;
        case 1:
            pkType = Reply;
            break;
        case 2:
            pkType = Cmd;
            break;
    }
    return Packet(data, dataLen, timestamp, aid, pkType);
}

Packet::Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, Packet::Type type) :
    _data((unsigned char*)data, (unsigned char*)data + len),
    _timestamp(timestamp),
    _aid(aid),
    _type(type){
}

std::vector<uint8_t> Packet::ToBuf() const {
    std::vector<uint8_t> ret;
    ret.reserve(HeaderSize + Length());
    uint64_t netTimestamp = swapToNetwork(_timestamp);
    ret.insert(ret.end(), (const char *)&netTimestamp, ((const char *)&netTimestamp) + sizeof(uint64_t));
    ret.push_back(_aid);
    uint8_t numericalType = 0;
    switch(_type) {
        case Other:
            numericalType = 0;
            break;
        case Reply:
            numericalType = 1;
            break;
        case Cmd:
            numericalType = 2;
            break;
    }
    ret.push_back(numericalType);
    ret.insert(ret.end(), _data.begin(), _data.end());
    return ret;
}

bool MpState::IsReady() const noexcept {
    return _sendFn != nullptr && _pollFn != nullptr;
}

void MpState::SetSendFn(retro_netpacket_send_t sendFn) noexcept {
    _sendFn = sendFn;
}

void MpState::SetPollFn(retro_netpacket_poll_receive_t pollFn) noexcept {
    _pollFn = pollFn;
}

void MpState::PacketReceived(const void *buf, size_t len, uint16_t client_id) noexcept {
    retro_assert(IsReady());
    Packet p = Packet::parsePk(buf, len);
    if(p.PacketType() == Packet::Type::Cmd) {
        _hostId = client_id;
        //retro::debug("Host client id is {}", client_id);
    }
    receivedPackets.push(std::move(p));
}

std::optional<Packet> MpState::NextPacket() noexcept {
    retro_assert(IsReady());
    if(receivedPackets.empty()) {
        _sendFn(RETRO_NETPACKET_FLUSH_HINT, NULL, 0, RETRO_NETPACKET_BROADCAST);
        _pollFn();
    }
    if(receivedPackets.empty()) {
        return std::nullopt;
    } else {
        Packet p = receivedPackets.front();
        receivedPackets.pop();
        return p;
    }
}

std::optional<Packet> MpState::NextPacketBlock() noexcept {
    retro_assert(IsReady());
    if (receivedPackets.empty()) {
        for(std::clock_t start = std::clock(); std::clock() < (start + (RECV_TIMEOUT_MS * CLOCKS_PER_SEC / 1000));) {
            _sendFn(RETRO_NETPACKET_FLUSH_HINT, NULL, 0, RETRO_NETPACKET_BROADCAST);
            _pollFn();
            if(!receivedPackets.empty()) {
                return NextPacket();
            }
        }
    } else {
        return NextPacket();
    }
    retro::debug("Timeout while waiting for packet");
    return std::nullopt;
}

void MpState::SendPacket(const Packet &p) noexcept {
    retro_assert(IsReady());
    uint16_t dest = RETRO_NETPACKET_BROADCAST;
    if(p.PacketType() == Packet::Type::Cmd) {
        _hostId = std::nullopt;
    }
    if(p.PacketType() == Packet::Type::Reply && _hostId.has_value()) {
        dest = _hostId.value();
    }
    _sendFn(RETRO_NETPACKET_UNSEQUENCED | RETRO_NETPACKET_UNRELIABLE | RETRO_NETPACKET_FLUSH_HINT, p.ToBuf().data(), p.Length() + HeaderSize, dest);
}


