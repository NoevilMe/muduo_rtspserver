#ifndef EA95BB86_1133_410D_A312_D3B63AE2A142
#define EA95BB86_1133_410D_A312_D3B63AE2A142

#define INTERLEAVED_FRAME_SIZE 4

namespace muduo_media {
namespace defs {

constexpr auto kBufPrependSize = 20;

constexpr auto kAppName = "Muduo Media Server";

constexpr auto kMimeTypeH264 = "H264";

enum class MediaCodecID {
    CODEC_ID_NONE = 0,
    CODEC_ID_H264 = 1,
    CODEC_ID_H265 = 2
};

constexpr auto kSdpMediaProtocol = "RTP/AVP";

/*================ RTP ==================*/
constexpr auto kRtpUnicast = "unicast";

constexpr auto kRtpOverUdp = "RTP/AVP";
constexpr auto kRtpOverUdpFull = "RTP/AVP/UDP";
constexpr auto kRtpOverTcp = "RTP/AVP/TCP";

enum class RtpTransportProtocol {
    kRtpTransportNone = 0,
    kRtpOverTcp = 1,
    kRtpOverUdp = 2,
    kRtpOverMulticast = 3
};

constexpr int kMediaFormatH264 = 96;
constexpr int kMediaTsDuration = 3600; // 9000/25

/*================ RTSP ==================*/
constexpr auto kRtspApplicationSdp = "application/sdp";
constexpr char kRtspInterleavedFrameMagic = '$';

} // namespace defs

using RtpTransProto = defs::RtpTransportProtocol;

} // namespace muduo_media

#endif /* EA95BB86_1133_410D_A312_D3B63AE2A142 */
