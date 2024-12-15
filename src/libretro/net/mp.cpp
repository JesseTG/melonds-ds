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
    uint8_t isReply = *(const uint8_t*)(indexableBuf + 9);
    retro_assert(isReply == 1 || isReply == 0);
    return Packet(data, dataLen, timestamp, aid, isReply == 1);
}

Packet::Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, bool isReply) :
    _data((unsigned char*)data, (unsigned char*)data + len),
    _timestamp(timestamp),
    _aid(aid),
    _isReply(isReply){
}

std::vector<uint8_t> Packet::ToBuf() const {
    std::vector<uint8_t> ret;
    ret.reserve(HeaderSize + Length());
    uint64_t netTimestamp = swapToNetwork(_timestamp);
    ret.insert(ret.end(), (const char *)&netTimestamp, ((const char *)&netTimestamp) + sizeof(uint64_t));
    ret.push_back(_aid);
    ret.push_back(_isReply);
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

void MpState::PacketReceived(const void *buf, size_t len) noexcept {
    retro_assert(IsReady());
    receivedPackets.push(Packet::parsePk(buf, len));
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

void MpState::SendPacket(const Packet &p) const noexcept {
    retro_assert(IsReady());
    _sendFn(RETRO_NETPACKET_UNSEQUENCED | RETRO_NETPACKET_UNRELIABLE | RETRO_NETPACKET_FLUSH_HINT, p.ToBuf().data(), p.Length() + HeaderSize, RETRO_NETPACKET_BROADCAST);
}


