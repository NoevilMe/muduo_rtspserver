#ifndef B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB
#define B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB

#include "multi_frame_rtp_sink.h"
#include "tinymuduo/net/udp_virtual_connection.h"

namespace muduo_media {

class H264VideoRtpSink : public MultiFrameRtpSink {
public:
    H264VideoRtpSink(
        const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn);
    virtual ~H264VideoRtpSink() = default;

    void Send(const void *data, int len,
              const std::shared_ptr<void> &add_data) override;

private:
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn_;
};

} // namespace muduo_media

#endif /* B7A4F4AA_EE3C_42C1_8257_3F5B5CF926CB */
