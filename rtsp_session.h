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

    void Setup(const std::string &track,
               const muduo::net::InetAddress &peer_rtp_addr,
               const muduo::net::InetAddress &peer_rtcp_addr,
               unsigned short &local_rtp_port, unsigned short &local_rtcp_port);

    int id() const { return id_; }

    void Play();
    void Teardown();

private:
    muduo::event_loop::EventLoop *loop_;
    std::weak_ptr<MediaSession> media_session_;

    int id_;

    std::shared_ptr<muduo::net::UdpVirtualConnection> rtp_conn_;
    std::shared_ptr<muduo::net::UdpVirtualConnection> rtcp_conn_;

    std::map<std::string, StreamStatePtr> states_;

    // std::vector<
};

} // namespace muduo_media

#endif /* ED091CCA_7ECE_478F_86D7_589C367B2D77 */
