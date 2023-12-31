#ifndef A829ACA4_FC85_4098_AA0A_92C3C824D67E
#define A829ACA4_FC85_4098_AA0A_92C3C824D67E

#include "file_media_subsession.h"

namespace muduo_media {

class H264FileSubsession : public FileMediaSubsession {

public:
    H264FileSubsession(const std::string &filename, unsigned int fps = 25,
                       unsigned int time_base = 90000);
    ~H264FileSubsession();

    std::string GetSdp() override;

    RtpSinkPtr
    NewRtpSink(const std::shared_ptr<muduo::net::TcpConnection> &tcp_conn,
               int8_t rtp_channel) override;

    RtpSinkPtr NewRtpSink(
        const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn)
        override;

    MultiFrameSourcePtr NewMultiFrameSouce() override;

private:
};

} // namespace muduo_media

#endif /* A829ACA4_FC85_4098_AA0A_92C3C824D67E */
