#pragma once
#include <cstdint>
#include <queue>
#include <optional>
#include <vector>
#include <libretro.h>

namespace MelonDsDs {
// timestamp, aid, and isReply, respectively.
constexpr size_t HeaderSize = sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint8_t);

class Packet {
public:
    enum Type {
        Reply, Cmd, Other
    };

    static Packet parsePk(const void *buf, uint64_t len);
    explicit Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, Packet::Type type);

    [[nodiscard]] uint64_t Timestamp() const noexcept {
        return _timestamp;
    };
    [[nodiscard]] uint8_t Aid() const noexcept {
        return _aid;
    };
    [[nodiscard]] Packet::Type PacketType() const noexcept {
        return _type;
    }
    [[nodiscard]] const void *Data() const noexcept {
        return _data.data();
    };
    [[nodiscard]] uint64_t Length() const noexcept {
        return _data.size();
    };

    std::vector<uint8_t> ToBuf() const;
private:
    uint64_t _timestamp;
    uint8_t _aid;
    Packet::Type _type;
    std::vector<uint8_t> _data;
};

class MpState {
public:
    void PacketReceived(const void *buf, size_t len, uint16_t client_id) noexcept;
    void SetSendFn(retro_netpacket_send_t sendFn) noexcept;
    void SetPollFn(retro_netpacket_poll_receive_t pollFn) noexcept;
    bool IsReady() const noexcept;
    void SendPacket(const Packet &p) noexcept;
    std::optional<Packet> NextPacket() noexcept;
    std::optional<Packet> NextPacketBlock() noexcept;
private:
    retro_netpacket_send_t _sendFn;
    retro_netpacket_poll_receive_t _pollFn;
    std::optional<uint16_t> _hostId;
    std::queue<Packet> receivedPackets;
};
}
