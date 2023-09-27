#ifndef ED091CCA_7ECE_478F_86D7_589C367B2D77
#define ED091CCA_7ECE_478F_86D7_589C367B2D77

#include "media/rtcp.h"
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

    void ParseTcpInterleavedFrameBody(uint8_t channel, const char *buf,
                                      size_t size);

private:
    enum ChannelOrPortType { kChannelRtp, kChannelRtcp, kPortRtp, kPortRtcp };
    struct ChannelOrPortStreamBinding {
        ChannelOrPortType cop_type;
        uint16_t cop_rtp;
        uint16_t cop_rtcp;
        std::string media_subsession;
        StreamStatePtr state;
    };

private:
    void SendTcpRtcpMessages(uint8_t channel, const RtcpMessageVector &msg);

    void SendUdpRtcpMessages(
        const std::shared_ptr<muduo::net::UdpVirtualConnection> &,
        const RtcpMessageVector &msg);

private:
    muduo::event_loop::EventLoop *loop_;
    std::weak_ptr<MediaSession> media_session_;
    int id_;

    muduo::net::TcpConnectionPtr tcp_conn_;

    std::map<uint8_t, std::shared_ptr<ChannelOrPortStreamBinding>>
        binding_states_;

    std::map<std::string, StreamStatePtr> states_;
};

using RtspSessionPtr = std::shared_ptr<RtspSession>;

} // namespace muduo_media

#endif /* ED091CCA_7ECE_478F_86D7_589C367B2D77 */
