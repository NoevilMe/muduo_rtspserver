#ifndef C6A67180_CD35_42B2_8219_E6D2F1932DF6
#define C6A67180_CD35_42B2_8219_E6D2F1932DF6

#include <stdint.h>

namespace muduo_media {

/**
 * RTCP固定头
 *
 *
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|    RC   |   PT=SR=200   |             length L          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct _rtcp_hdr_t {
    uint8_t rc : 5; /* reception report count */
    uint8_t p : 1;  /* padding */
    uint8_t v : 2;  /* version */

    uint8_t pt;      /* packet type */
    
    uint16_t length; /* pkt len in words, w/o this word */
} RtcpHeader;

/**
 * RTCP报告类型
 */
enum class RtcpPacketType {
    RTCP_FIR = 192,     // 关键帧请求, RFC2032
    RTCP_NACK = 193,    // 丢包重传, RFC2032
    RTCP_SMPTETC = 194, // RFC5484
    RTCP_IJ = 195,      // RFC5450

    RTCP_SR = 200,   // 发送者报告
    RTCP_RR = 201,   // 接受者报告
    RTCP_SDES = 202, // 源点描述
    RTCP_BYE = 203,  // 结束传输
    RTCP_APP = 204,  // 特定应用

    RTCP_RTPFB = 205, // RTP Feedback, RFC4585
    RTCP_PSFB = 206,  // PS Feedback, RFC4585
    RTCP_XR = 207,    // RFC3611
    RTCP_AVB = 208,
    RTCP_RSI = 209,   // RFC5760
    RTCP_TOKEN = 210, // RFC6284
    RTCP_IDMS = 211,  // RFC7272
    RTCP_RGRS = 212,  // RFC8861

    RTCP_LIMIT = 223,
};

} // namespace muduo_media

#endif /* C6A67180_CD35_42B2_8219_E6D2F1932DF6 */
