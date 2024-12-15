#pragma once
#include <cstdint>
#include <queue>
#include <optional>
#include <vector>
#include <libretro.h>
#include <ctime>

namespace MelonDsDs {
// timestamp, aid, and isReply, respectively.
constexpr size_t HeaderSize = sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint8_t);

class Packet {
public:
    static Packet parsePk(const void *buf, uint64_t len);
    explicit Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, bool isReply);

    [[nodiscard]] uint64_t Timestamp() const noexcept {
        return _timestamp;
    };
    [[nodiscard]] uint8_t Aid() const noexcept {
        return _aid;
    };
    [[nodiscard]] bool IsReply() const noexcept {
        return _isReply;
    };
    [[nodiscard]] const void *Data() const noexcept {
        return _data.data();
    };
    [[nodiscard]] uint64_t Length() const noexcept {
        return _data.size();
    };
    [[nodiscard]] uint64_t TimeDeltaUs() const noexcept;

    std::vector<uint8_t> ToBuf() const;
private:
    uint64_t _timestamp;
    uint8_t _aid;
    bool _isReply;
    std::vector<uint8_t> _data;
    std::clock_t _recvTime;
};

class MpState {
public:
    void PacketReceived(const void *buf, size_t len) noexcept;
    void SetSendFn(retro_netpacket_send_t sendFn) noexcept;
    void SetPollFn(retro_netpacket_poll_receive_t pollFn) noexcept;
    bool IsReady() const noexcept;
    void SendPacket(const Packet &p) const noexcept;
    std::optional<Packet> NextPacket() noexcept;
    std::optional<Packet> NextPacketBlock() noexcept;
private:
    retro_netpacket_send_t _sendFn;
    retro_netpacket_poll_receive_t _pollFn;
    std::queue<Packet> receivedPackets;
};
}
