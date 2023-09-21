#ifndef B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB
#define B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB

#include "multi_frame_rtp_sink.h"
#include "net/tcp_connection.h"
#include "net/udp_virtual_connection.h"

namespace muduo_media {

class H264VideoRtpSink : public MultiFrameRtpSink {
public:
    H264VideoRtpSink(const muduo::net::TcpConnectionPtr &tcp_conn,
                     int8_t rtp_channel);

    H264VideoRtpSink(const muduo::net::UdpVirtualConnectionPtr &udp_conn);

    virtual ~H264VideoRtpSink();

    void Send(const unsigned char *data, int len,
              const std::shared_ptr<void> &add_data) override;

private:
    void SendOverTcp(const unsigned char *data, int len,
                     const std::shared_ptr<void> &add_data);

    void SendOverUdp(const unsigned char *data, int len,
                     const std::shared_ptr<void> &add_data);

private:
    muduo::net::TcpConnectionPtr tcp_conn_;
    int8_t rtp_channel_;

    muduo::net::UdpVirtualConnectionPtr udp_conn_;

    uint16_t init_seq_;
};

} // namespace muduo_media

#endif /* B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB */
