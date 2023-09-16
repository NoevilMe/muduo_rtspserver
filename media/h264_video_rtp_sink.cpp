#include "h264_video_rtp_sink.h"
namespace muduo_media {
H264VideoRtpSink::H264VideoRtpSink(
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn)
    : udp_conn_(udp_conn) {}

void H264VideoRtpSink::Send(const void *data, int len,
                            const std::shared_ptr<void> &add_data) {
    udp_conn_->Send(data, len);
}

} // namespace muduo_media