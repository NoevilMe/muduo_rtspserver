#ifndef D79D44DE_4579_4145_9AF4_4E0BBD7E75B3
#define D79D44DE_4579_4145_9AF4_4E0BBD7E75B3

namespace muduo_media {

constexpr auto kRtpTransportProtocolUdp = "RTP/AVP";
constexpr auto kRtpTransportProtocolUdpFull = "RTP/AVP/UDP";
constexpr auto kRtpTransportProtocolTcp = "RTP/AVP/TCP";

constexpr auto kRtpUnicast = "unicast";

enum class RtpTransportProtocol {
    kRtpTransportNone = 0,
    kRtpOverTcp = 1,
    kRtpOverUdp = 2,
    kRtpOverMulticast = 3
};

constexpr auto kRtspApplicationSdp = "application/sdp";

constexpr char kRtspInterleavedFrameMagic = '$';

struct RtspInterleavedFrame {
    uint8_t magic;
    uint8_t channel;
    uint16_t length;
};

} // namespace muduo_media

#endif /* D79D44DE_4579_4145_9AF4_4E0BBD7E75B3 */
