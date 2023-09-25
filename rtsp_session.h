#ifndef ED091CCA_7ECE_478F_86D7_589C367B2D77
#define ED091CCA_7ECE_478F_86D7_589C367B2D77

#include "media_session.h"
#include "net/udp_virtual_connection.h"
#include "stream_state.h"

#include <memory>

namespace muduo_media {

class RtpConnection;

class RtspSession {
public:
    RtspSession(muduo::event_loop::EventLoop *loop,
                const std::weak_ptr<MediaSession> &media_session);
    ~RtspSession();

    // over udp
    void Setup(const std::string &track,
               const muduo::net::InetAddress &peer_rtp_addr,
               const muduo::net::InetAddress &peer_rtcp_addr,
               unsigned short &local_rtp_port, unsigned short &local_rtcp_port);

    // over tcp
    void Setup(const std::string &track,
               const muduo::net::TcpConnectionPtr &tcp_conn, int8_t rtp_channel,
               int8_t rtcp_channel);

    int id() const { return id_; }

    void Play();
    void Teardown();

private:
    void OnRtpMessage(const muduo::net::UdpServerPtr &, muduo::net::Buffer *,
                      struct sockaddr_in6 *, muduo::event_loop::Timestamp);

    void OnRtcpMessage(const muduo::net::UdpServerPtr &, muduo::net::Buffer *,
                       struct sockaddr_in6 *, muduo::event_loop::Timestamp);

private:
    muduo::event_loop::EventLoop *loop_;
    std::weak_ptr<MediaSession> media_session_;

    muduo::net::TcpConnectionPtr tcp_conn_;

    int id_;

    std::shared_ptr<muduo::net::UdpVirtualConnection> rtp_conn_;
    std::shared_ptr<muduo::net::UdpVirtualConnection> rtcp_conn_;

    std::map<std::string, StreamStatePtr> states_;
};

using RtspSessionPtr = std::shared_ptr<RtspSession>;

} // namespace muduo_media

#endif /* ED091CCA_7ECE_478F_86D7_589C367B2D77 */
