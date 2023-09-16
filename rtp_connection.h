#ifndef C3EA578B_21D5_4EEA_A7F0_1244C148C6AA
#define C3EA578B_21D5_4EEA_A7F0_1244C148C6AA

#include "net/inet_socket.h"

#include <memory>

namespace muduo_media {

class RtpConnection {
public:
    RtpConnection() = default;
    virtual ~RtpConnection() = default;

    // UDP
    virtual bool Bind(unsigned short &local_rtp_port,
                      unsigned short &local_rtcp_port) {
        return false;
    };
};

class RtpUdpConnection : public RtpConnection {
public:
    RtpUdpConnection(unsigned short peer_rtp_port,
                     unsigned short peer_rtcp_port);
    ~RtpUdpConnection();

    bool Bind(unsigned short &local_rtp_port,
              unsigned short &local_rtcp_port) override;

private:
    unsigned short peer_rtp_port_;
    unsigned short peer_rtcp_port_;

    std::unique_ptr<muduo::net::Socket> rtp_socket_;
    std::unique_ptr<muduo::net::Socket> rtcp_socket_;
};

class RtpTcpConnection : public RtpConnection {
public:
    RtpTcpConnection();
    ~RtpTcpConnection();
};

} // namespace muduo_media

#endif /* C3EA578B_21D5_4EEA_A7F0_1244C148C6AA */
