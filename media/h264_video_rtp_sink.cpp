#include "h264_video_rtp_sink.h"
#include "logger/logger.h"
#include "rtp.h"

namespace muduo_media {

H264VideoRtpSink::H264VideoRtpSink(
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn)
    : udp_conn_(udp_conn) {
    LOG_DEBUG << "H264VideoRtpSink::ctor at " << this;
}

H264VideoRtpSink::~H264VideoRtpSink() {
    LOG_DEBUG << "H264VideoRtpSink::dtor at " << this;
}

void H264VideoRtpSink::Send(const void *data, int len,
                            const std::shared_ptr<void> &add_data) {

    RtpHeader *header = (RtpHeader *)add_data.get();

    std::unique_ptr<unsigned char[]> new_buf(
        new unsigned char[RTP_HEADER_SIZE + len]);

    memcpy(new_buf.get(), header, RTP_HEADER_SIZE);
    memcpy(new_buf.get() + RTP_HEADER_SIZE, data, len);

    udp_conn_->Send(new_buf.get(), RTP_HEADER_SIZE + len);
}

} // namespace muduo_media