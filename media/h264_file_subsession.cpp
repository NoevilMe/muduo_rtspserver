#include "h264_file_subsession.h"
#include "h264_file_source.h"
#include "h264_video_rtp_sink.h"

#include <cstring>

namespace muduo_media {
H264FileSubsession::H264FileSubsession(const std::string &filename)
    : FileMediaSubsession(filename) {}

H264FileSubsession::~H264FileSubsession() {}

std::string H264FileSubsession::GetSdp() {
    char media_sdp[200] = {0};
    snprintf(media_sdp, sizeof(media_sdp),
             "m=video 0 RTP/AVP 96\r\n"
             "a=rtpmap:96 H264/90000\r\n"
             "a=framerate:25\r\n"
             "a=control:%s\r\n",
             TrackId().data());
    return media_sdp;
}

RtpSinkPtr H264FileSubsession::NewRtpSink(
    const std::shared_ptr<muduo::net::TcpConnection> &tcp_conn,
    int8_t rtp_channel) {
    return std::make_shared<H264VideoRtpSink>(tcp_conn, rtp_channel);
}

RtpSinkPtr H264FileSubsession::NewRtpSink(
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn) {
    return std::make_shared<H264VideoRtpSink>(udp_conn);
}

MultiFrameSourcePtr H264FileSubsession::NewMultiFrameSouce() {

    FILE *file = fopen(filename_.data(), "rb");

    std::shared_ptr<H264FileSource> filesource(new H264FileSource(file));

    return filesource;
}

} // namespace muduo_media