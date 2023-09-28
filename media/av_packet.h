#ifndef BEA8DF55_A35C_4210_A032_E4E70144E119
#define BEA8DF55_A35C_4210_A032_E4E70144E119

#include <cstdint>
#include <memory>

namespace muduo_media {

struct AVPacket {
    std::shared_ptr<uint8_t[]> buffer; /* 帧数据 */
    uint32_t size = 0;                 /* 帧大小 */
    uint32_t prepend_size = 0;         /* 预留前置空间*/
    uint8_t type = 0;                  /* 帧类型 */
    // uint32_t timestamp = 0;            /* 时间戳 */
};

struct AVPacketInfo {
    uint32_t ssrc = 0;
    uint32_t timestamp = 0;
    uint8_t payload_type;
    // bool last
};

typedef enum {
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,
} H264NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} H264NaluPriority;

typedef struct {
    uint8_t statcode_length;   //! 4 for parameter sets and first slice in
                               //! picture, 3 for everything else (suggested)
    uint8_t forbidden_bit;     //! should be always FALSE
    uint8_t nal_reference_idc; //! NALU_PRIORITY_xxxx
    uint8_t nal_unit_type;     //! NALU_TYPE_xxxx

    size_t len;      //! Length of the NAL unit (Excluding the start code, which
                     //! does not belong to the NALU)
    size_t max_size; //! Nal Unit Buffer size
    size_t prepend_size; //! not used size in the front of buf
    std::shared_ptr<unsigned char[]>
        buf; //! contains the first byte followed by the EBSP

    void AppendData(unsigned char *data_buf, size_t data_len);
} H264Nalu;

} // namespace muduo_media

#endif /* BEA8DF55_A35C_4210_A032_E4E70144E119 */
