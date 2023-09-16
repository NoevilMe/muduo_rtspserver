#ifndef F3741B40_B876_441F_9130_91995A20EA31
#define F3741B40_B876_441F_9130_91995A20EA31

#include "net/buffer.h"

#include <memory>
#include <stdint.h>

#define RTP_VESION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PAYLOAD_SIZE 1400 // 最大1460，  1500(MTU)-20(IP)-8(UDP)-12(RTP)

#define RTP_PACKET_PREPEND_SIZE 20

static_assert(RTP_PACKET_PREPEND_SIZE > RTP_HEADER_SIZE);

/*  https://www.rfc-editor.org/rfc/rfc3550.txt
 *
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
struct RtpHeader {
    /* byte 0 位域表示法*/
    // CSRC计数器（CC）：CSRC计数器，占4位，指示固定头部后面跟着的CSRC
    // 标识符的个数
    uint8_t csrcLen : 4;
    // 扩展位（X）：扩展标志，占1位，如果X=1，则在RTP固定头部后面就跟有一个扩展头部
    uint8_t extension : 1;
    // 填充位（P）：填充标志，占1位，如果P=1，则该RTP包的尾部就包含附加的填充字节
    uint8_t padding : 1;
    // 版本号（V）：用来标志使用的RTP版本，占2位，当前协议版本号为2
    uint8_t version : 2;

    /* byte 1 */
    // 载荷类型（PayloadType）：
    // 有效荷载类型，占7位，用于说明RTP报文中有效载荷的类型
    uint8_t payloadType : 7;
    // 标记位（M）：标记，占1位，一般而言，对于视频，标记一帧的结束；对于音频，标记会话的开始
    uint8_t marker : 1;

    /* bytes 2,3 */
    // 序列号（SN）：占16位，用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1
    uint16_t seq;

    /* bytes 4-7 */
    // 时间戳(Timestamp): 占32位，记录了该包中数据的第一个字节的采样时刻
    uint32_t timestamp;

    /* bytes 8-11 */
    // 同步源标识符(SSRC)：占32位，用于标识同步信源，同步源就是指RTP包流的来源。在同一个RTP会话中不能有两个相同的SSRC值
    uint32_t ssrc;
};

struct RtpPacket {
    RtpPacket()
        : buffer(new muduo::net::Buffer(RTP_PACKET_PREPEND_SIZE,
                                        RTP_MAX_PAYLOAD_SIZE)) {}

    std::shared_ptr<muduo::net::Buffer> buffer;
    uint32_t size;
    uint32_t timestamp;
    uint8_t last_marker;
};

#endif /* F3741B40_B876_441F_9130_91995A20EA31 */
