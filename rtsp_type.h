#ifndef D79D44DE_4579_4145_9AF4_4E0BBD7E75B3
#define D79D44DE_4579_4145_9AF4_4E0BBD7E75B3

namespace muduo_media {

constexpr auto kRtpMediaProtoUdp = "RTP/AVP";

constexpr auto kRtpUnicast = "unicast";

typedef enum StreamingMode {
  RTP_UDP,
  RTP_TCP,
  RAW_UDP
} StreamingMode;

} // namespace muduo_media

#endif /* D79D44DE_4579_4145_9AF4_4E0BBD7E75B3 */
