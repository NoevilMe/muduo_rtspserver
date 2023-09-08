#ifndef ED091CCA_7ECE_478F_86D7_589C367B2D77
#define ED091CCA_7ECE_478F_86D7_589C367B2D77

#include "net/udp_virtual_connection.h"

#include <memory>

namespace rtsp {

class RtpConnection;

class RtspSession {
public:
    RtspSession(muduo::event_loop::EventLoop *loop);
    ~RtspSession();

    void Setup(const muduo::net::InetAddress &peer_rtp_addr,
               const muduo::net::InetAddress &peer_rtcp_addr,
               unsigned short &local_rtp_port, unsigned short &local_rtcp_port);

    int id() const { return id_; }

private:
    muduo::event_loop::EventLoop *loop_;

    int id_;

    std::shared_ptr<muduo::net::UdpVirtualConnection> rtp_conn_;
    std::shared_ptr<muduo::net::UdpVirtualConnection> rtcp_conn_;
};

} // namespace rtsp

#endif /* ED091CCA_7ECE_478F_86D7_589C367B2D77 */
