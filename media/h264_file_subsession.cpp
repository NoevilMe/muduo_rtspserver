#include "h264_file_subsession.h"
#include "defs.h"
#include "h264_file_source.h"
#include "h264_video_rtp_sink.h"

#include <cstring>

namespace muduo_media {
H264FileSubsession::H264FileSubsession(const std::string &filename,
                                       unsigned int fps, unsigned int time_base)
    : FileMediaSubsession(filename, fps, time_base) {
    set_payload_type(defs::kMediaFormatH264);
}

H264FileSubsession::~H264FileSubsession() {}

std::string H264FileSubsession::GetSdp() {
    char media_sdp[200] = {0};
    snprintf(media_sdp, sizeof(media_sdp),
             "m=video 0 %s %hu\r\n"
             "a=rtpmap:%hu %s/%u\r\n"
             "a=framerate:%u\r\n"
             "a=control:%s\r\n",
             defs::kSdpMediaProtocol, payload_type_, payload_type_,
             defs::kMimeTypeH264, time_base_, fps_, TrackId().data());
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