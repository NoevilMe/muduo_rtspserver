#include "h264_video_rtp_sink.h"
#include "defs.h"
#include "eventloop/endian.h"
#include "logger/logger.h"
#include "rtp.h"

#include <random>
#include <stdexcept>

namespace muduo_media {
H264VideoRtpSink::H264VideoRtpSink(const muduo::net::TcpConnectionPtr &tcp_conn,
                                   int8_t rtp_channel)
    : tcp_conn_(tcp_conn), rtp_channel_(rtp_channel), udp_conn_(nullptr) {

    std::random_device rd;
    init_seq_ = rd() & 0xFF; // limited
    LOG_DEBUG << "H264VideoRtpSink::ctor at " << this;
}

H264VideoRtpSink::H264VideoRtpSink(
    const muduo::net::UdpVirtualConnectionPtr &udp_conn)
    : tcp_conn_(nullptr), rtp_channel_(-1), udp_conn_(udp_conn) {
    // udp_conn_->SetSendBufSize(128 * 1024);

    std::random_device rd;
    init_seq_ = rd() & 0xFF; // limited
    LOG_DEBUG << "H264VideoRtpSink::ctor at " << this;
}

H264VideoRtpSink::~H264VideoRtpSink() {
    LOG_DEBUG << "H264VideoRtpSink::dtor at " << this;
}

void H264VideoRtpSink::Send(const unsigned char *data, int len,
                            const std::shared_ptr<void> &info) {
    if (tcp_conn_) {
        SendOverTcp(data, len, info);
    } else {
        SendOverUdp(data, len, info);
    }
}

void H264VideoRtpSink::Send(const AVPacket &pkt, const AVPacketInfo &info) {

    std::shared_ptr<RtpHeader> rtp_header = std::make_shared<RtpHeader>();
    ::bzero(rtp_header.get(), sizeof(RtpHeader));
    rtp_header->version = RTP_VESION;
    rtp_header->padding = 0;
    rtp_header->extension = 0;
    rtp_header->csrcLen = 0;

    /* marker代表本包是一系列RTP包的结尾，这些RTP包携带的是分片数据，一般是I帧，因为单帧太大进行分片，每个RTP包发送一片，marker=false，最后一包marker=ture。
    而如果帧数据很小，一般是BP帧，一帧或者几帧被一个RTP包发送，则marker=true。也即marker代表完整帧（一帧或几帧）
    不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始
    当多个RTP包携带1帧数据时，前面的RTP包的marker标志设置为0，最后一个RTP包marker标志设置为1。
    */
    rtp_header->marker = 1; //  最后一帧
    rtp_header->payloadType = info.payload_type;
    rtp_header->seq = 0; // rtp sink will modify this value
    rtp_header->timestamp =
        muduo::HostToNetwork32(info.timestamp); // 需要根据源计算 htonl()
    rtp_header->ssrc = muduo::HostToNetwork32(info.ssrc); // 信号源id

    if (tcp_conn_) {
        SendOverTcp(pkt, rtp_header);
    } else {
        SendOverUdp(pkt, rtp_header);
    }
}

void H264VideoRtpSink::SendOverTcp(const AVPacket &pkt,
                                   const std::shared_ptr<RtpHeader> &header) {
    /*
     *   0 1 2 3 4 5 6 7 8 9
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |F|NRI|  Type   | a single NAL unit ... |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

    assert(pkt.prepend_size >= INTERLEAVED_FRAME_SIZE + RTP_HEADER_SIZE);

    // Fill header in the front of buffer, because there is a prepend space.
    uint8_t *pdata_start =
        pkt.buffer.get() +
        (pkt.prepend_size - INTERLEAVED_FRAME_SIZE - RTP_HEADER_SIZE);

    // Interleaved Frame
    uint32_t rtp_len = RTP_HEADER_SIZE + pkt.size;
    pdata_start[0] = '$';
    pdata_start[1] = (uint8_t)rtp_channel_;
    pdata_start[2] = (uint8_t)((rtp_len & 0xFF00) >> 8);
    pdata_start[3] = (uint8_t)(rtp_len & 0xFF);

    // Fill header
    LOG_TRACE << "seq " << init_seq_;
    header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长

    uint8_t *pdata = pdata_start + INTERLEAVED_FRAME_SIZE;
    memcpy(pdata, header.get(), RTP_HEADER_SIZE);

    uint32_t packet_len = INTERLEAVED_FRAME_SIZE + rtp_len;
    tcp_conn_->Send(pdata_start, packet_len);

    ++packets_;
    octets_ += packet_len;
}
void H264VideoRtpSink::SendOverUdp(const AVPacket &pkt,
                                   const std::shared_ptr<RtpHeader> &header) {

    if (pkt.size <= RTP_MAX_PAYLOAD_SIZE) {
        // if (true) {
        /*
         *   0 1 2 3 4 5 6 7 8 9
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|NRI|  Type   | a single NAL unit ... |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        assert(pkt.prepend_size >= RTP_HEADER_SIZE);

        // Fill header in the front of buffer, because there is a prepend space.
        uint8_t *pdata_start =
            pkt.buffer.get() + (pkt.prepend_size - RTP_HEADER_SIZE);

        LOG_TRACE << "send seq " << init_seq_;
        header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
        memcpy(pdata_start, header.get(), RTP_HEADER_SIZE);

        uint32_t packet_len = RTP_HEADER_SIZE + pkt.size;
        udp_conn_->Send(pdata_start, packet_len);

        ++packets_;
        octets_ += packet_len;

    } else {
        // 分片打包的话，那么在RTP载荷开始有两个字节的信息，然后再是NALU的内容
        /*
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         *
         * 前三个bit位就是NALU头的前面三个bit位；后五位的TYPE就是NALU的FU-A类型28
         * 1. 装载FU payload部分时候，需要去掉nalu的header（第一个字节）
         * 2. 一般I帧前面发送sps和pps，时间戳和I帧相同
         * 3. h264的采样率固定是9000hz
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         *  S：标记该分片打包的第一个RTP包
         *  E：比较该分片打包的最后一个RTP包
         *  R: 保留位必须设置为0
         *  Type：NALU的Type, 取1-23的那个值，表示 NAL单元荷载类型定义
         */

        const unsigned char *pdata = pkt.buffer.get() + pkt.prepend_size;
        int data_len = pkt.size;

        unsigned char FU_A[RTP_FU_A_HEAD_LEN] = {0};
        // FU Indicator
        FU_A[0] = (pdata[0] & 0xE0) | RTP_FU_A_TYPE;
        // FU Header
        FU_A[1] = 0x80 | (pdata[0] & 0x1F);

        ++pdata;
        data_len -= 1;

        while (data_len + RTP_FU_A_HEAD_LEN > RTP_MAX_PAYLOAD_SIZE) {
            std::unique_ptr<unsigned char[]> new_buf(
                new unsigned char[RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE]);

            LOG_TRACE << "send seq " << init_seq_;
            header->marker = 0;
            header->seq =
                muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
            memcpy(new_buf.get(), header.get(), RTP_HEADER_SIZE);
            memcpy(new_buf.get() + RTP_HEADER_SIZE, &FU_A, RTP_FU_A_HEAD_LEN);
            memcpy(new_buf.get() + RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN, pdata,
                   RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN);

            udp_conn_->Send(new_buf.get(),
                            RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE);

            ++packets_;
            octets_ += RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE;

            pdata += RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN;
            data_len -= RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN;

            // set S 0
            FU_A[1] &= ~0x80;
        }

        // last RTP piece
        FU_A[1] |= 0x40;
        std::unique_ptr<unsigned char[]> new_buf(
            new unsigned char[RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len]);

        header->marker = 1;
        LOG_TRACE << "send seq " << init_seq_;
        header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
        memcpy(new_buf.get(), header.get(), RTP_HEADER_SIZE);
        memcpy(new_buf.get() + RTP_HEADER_SIZE, &FU_A, RTP_FU_A_HEAD_LEN);
        memcpy(new_buf.get() + RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN, pdata,
               data_len);

        udp_conn_->Send(new_buf.get(),
                        RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len);

        ++packets_;
        octets_ += RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len;
    }
}

void H264VideoRtpSink::SendOverTcp(const unsigned char *data, int len,
                                   const std::shared_ptr<void> &add_data) {

    /*
     *   0 1 2 3 4 5 6 7 8 9
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |F|NRI|  Type   | a single NAL unit ... |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

    std::unique_ptr<unsigned char[]> new_buf(
        new unsigned char[INTERLEAVED_FRAME_SIZE + RTP_HEADER_SIZE + len]);

    uint8_t *ptr = new_buf.get();
    ptr[0] = '$';
    ptr[1] = (uint8_t)rtp_channel_;
    ptr[2] = (uint8_t)(((RTP_HEADER_SIZE + len) & 0xFF00) >> 8);
    ptr[3] = (uint8_t)((RTP_HEADER_SIZE + len) & 0xFF);

    LOG_TRACE << "seq " << init_seq_;
    RtpHeader *header = (RtpHeader *)add_data.get();
    header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长

    memcpy(new_buf.get() + INTERLEAVED_FRAME_SIZE, header, RTP_HEADER_SIZE);
    memcpy(new_buf.get() + INTERLEAVED_FRAME_SIZE + RTP_HEADER_SIZE, data, len);

    tcp_conn_->Send(new_buf.get(),
                    INTERLEAVED_FRAME_SIZE + RTP_HEADER_SIZE + len);

    ++packets_;
    octets_ += INTERLEAVED_FRAME_SIZE + RTP_HEADER_SIZE + len;
}

void H264VideoRtpSink::SendOverUdp(const unsigned char *data, int len,
                                   const std::shared_ptr<void> &add_data) {

    RtpHeader *header = (RtpHeader *)add_data.get();

    if (len <= RTP_MAX_PAYLOAD_SIZE) {
        // if (true) {
        /*
         *   0 1 2 3 4 5 6 7 8 9
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|NRI|  Type   | a single NAL unit ... |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        std::unique_ptr<unsigned char[]> new_buf(
            new unsigned char[RTP_HEADER_SIZE + len]);

        LOG_TRACE << "send seq " << init_seq_;
        header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
        memcpy(new_buf.get(), header, RTP_HEADER_SIZE);
        memcpy(new_buf.get() + RTP_HEADER_SIZE, data, len);

        udp_conn_->Send(new_buf.get(), RTP_HEADER_SIZE + len);

        ++packets_;
        octets_ += RTP_HEADER_SIZE + len;

    } else {
        // 分片打包的话，那么在RTP载荷开始有两个字节的信息，然后再是NALU的内容
        /*
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         *
         * 前三个bit位就是NALU头的前面三个bit位；后五位的TYPE就是NALU的FU-A类型28
         * 1. 装载FU payload部分时候，需要去掉nalu的header（第一个字节）
         * 2. 一般I帧前面发送sps和pps，时间戳和I帧相同
         * 3. h264的采样率固定是9000hz
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         *  S：标记该分片打包的第一个RTP包
         *  E：比较该分片打包的最后一个RTP包
         *  R: 保留位必须设置为0
         *  Type：NALU的Type, 取1-23的那个值，表示 NAL单元荷载类型定义
         */

        const unsigned char *pdata = data;
        int data_len = len;

        unsigned char FU_A[RTP_FU_A_HEAD_LEN] = {0};
        // FU Indicator
        FU_A[0] = (pdata[0] & 0xE0) | RTP_FU_A_TYPE;
        // FU Header
        FU_A[1] = 0x80 | (pdata[0] & 0x1F);

        ++pdata;
        data_len -= 1;

        while (data_len + RTP_FU_A_HEAD_LEN > RTP_MAX_PAYLOAD_SIZE) {
            std::unique_ptr<unsigned char[]> new_buf(
                new unsigned char[RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE]);

            LOG_TRACE << "send seq " << init_seq_;
            header->marker = 0;
            header->seq =
                muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
            memcpy(new_buf.get(), header, RTP_HEADER_SIZE);
            memcpy(new_buf.get() + RTP_HEADER_SIZE, &FU_A, RTP_FU_A_HEAD_LEN);
            memcpy(new_buf.get() + RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN, pdata,
                   RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN);

            udp_conn_->Send(new_buf.get(),
                            RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE);

            ++packets_;
            octets_ += RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE;

            pdata += RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN;
            data_len -= RTP_MAX_PAYLOAD_SIZE - RTP_FU_A_HEAD_LEN;

            // set S 0
            FU_A[1] &= ~0x80;
        }

        // last RTP piece
        FU_A[1] |= 0x40;
        std::unique_ptr<unsigned char[]> new_buf(
            new unsigned char[RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len]);

        header->marker = 1;
        LOG_TRACE << "send seq " << init_seq_;
        header->seq = muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长
        memcpy(new_buf.get(), header, RTP_HEADER_SIZE);
        memcpy(new_buf.get() + RTP_HEADER_SIZE, &FU_A, RTP_FU_A_HEAD_LEN);
        memcpy(new_buf.get() + RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN, pdata,
               data_len);

        udp_conn_->Send(new_buf.get(),
                        RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len);

        ++packets_;
        octets_ += RTP_HEADER_SIZE + RTP_FU_A_HEAD_LEN + data_len;
    }
}

} // namespace muduo_media
