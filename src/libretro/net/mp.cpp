#include "mp.hpp"
#include <libretro.h>
#include <retro_assert.h>
#define TIMEOUT_FOR_POLL 16
using namespace MelonDsDs;

uint64_t swapToNetwork(uint64_t n) {
#ifdef BIG_ENDIAN
    return n;
#else
    char *d = (char*)&n;
    uint64_t ret;
    char *r = (char*)&ret;
    r[7] = d[0];
    r[6] = d[1];
    r[5] = d[2];
    r[4] = d[3];
    r[3] = d[4];
    r[2] = d[5];
    r[1] = d[6];
    r[0] = d[7];
    return ret;
#endif
}

Packet Packet::parsePk(const void *buf, uint64_t len) {
    // Necessary because arithmetic on void* is forbidden
    const char *indexableBuf = (const char *)buf;
    const char *data = indexableBuf + HeaderSize;
    retro_assert(len > HeaderSize);
    size_t dataLen = len - HeaderSize;
    uint64_t timestamp = swapToNetwork(*(const uint64_t*)(indexableBuf));
    uint8_t aid = *(const uint8_t*)(indexableBuf + 4);
    uint8_t isReply = *(const uint8_t*)(indexableBuf + 5);
    retro_assert(isReply == 1 || isReply == 0);
    return Packet(data, dataLen, timestamp, aid, isReply == 1);
}

Packet::Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, bool isReply) {
    // Necessary because arithmetic on void* is forbidden
    const char *indexableData = (const char *)data;
    _buf.reserve(HeaderSize + len);
    // We use network byte order (big endian)
    _buf.push_back(swapToNetwork(timestamp));
    _buf.push_back(aid);
    _buf.push_back(isReply ? 1 : 0);
    _buf.insert(_buf.end(), indexableData, indexableData + len);
}

uint64_t Packet::Timestamp() {
    return swapToNetwork(*(const uint64_t*)(_buf.data()));
}

uint8_t Packet::Aid() {
    return *(const uint8_t*)(_buf.data() + 4);
}

bool Packet::IsReply() {
    return *(const uint8_t*)(_buf.data() + 5) == 1;
}

const void *Packet::Data() {
    return _buf.data() + HeaderSize;
}

uint64_t Packet::Length() {
    return _buf.size() - HeaderSize;
}

const void *Packet::ToBuf() {
    return _buf.data();
}

bool MpState::IsReady() {
    return _sendFn != nullptr && _pollFn != nullptr;
}

void MpState::SetSendFn(retro_netpacket_send_t sendFn) {
    _sendFn = sendFn;
}

void MpState::SetPollFn(retro_netpacket_poll_receive_t pollFn) {
    _pollFn = pollFn;
}

void MpState::PacketReceived(const void *buf, size_t len) {
    receivedPackets.push(Packet::parsePk(buf, len));
}

std::optional<Packet> MpState::NextPacket() {
    if(receivedPackets.empty()) {
        return std::nullopt;
    } else {
        Packet p = receivedPackets.front();
        receivedPackets.pop();
        return p;
    }
}

std::optional<Packet> MpState::NextPacketBlock() {
    if (receivedPackets.empty()) {
        int i;
        for(i = 0; i < TIMEOUT_FOR_POLL; i++) {
            _pollFn();
            if(!receivedPackets.empty()) {
                break;
            }
        }
        if(i == TIMEOUT_FOR_POLL) {
            return std::nullopt;
        }
    }
    Packet p = receivedPackets.front();
    receivedPackets.pop();
    return p;
}

void MpState::SendPacket(Packet p) {
    _sendFn(RETRO_NETPACKET_UNRELIABLE | RETRO_NETPACKET_UNSEQUENCED, p.ToBuf(), p.Length() + HeaderSize, RETRO_NETPACKET_BROADCAST);
}


