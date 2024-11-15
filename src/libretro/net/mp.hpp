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
    static Packet parsePk(const void *buf, uint64_t len);
    explicit Packet(const void *data, uint64_t len, uint64_t timestamp, uint8_t aid, bool isReply);
    
    uint64_t Timestamp();
    uint8_t Aid();
    bool IsReply();
    const void *Data();
    uint64_t Length();
    
    const void *ToBuf();
private:
    std::vector<uint8_t> _buf;
};

class MpState {
public:
    void PacketReceived(const void *buf, size_t len);
    void SetSendFn(retro_netpacket_send_t sendFn);
    void SetPollFn(retro_netpacket_poll_receive_t pollFn);
    bool IsReady();
    void SendPacket(Packet p);
    std::optional<Packet> NextPacket();
    std::optional<Packet> NextPacketBlock();
private:
    retro_netpacket_send_t _sendFn;
    retro_netpacket_poll_receive_t _pollFn;
    std::queue<Packet> receivedPackets;
};
}
