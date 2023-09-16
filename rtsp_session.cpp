#include "rtsp_session.h"
#include "logger/logger.h"
#include "rtp_connection.h"
#include "rtsp_stream_state.h"

#include <random>

namespace muduo_media {
RtspSession::RtspSession(muduo::event_loop::EventLoop *loop,
                         const std::weak_ptr<MediaSession> &media_session)
    : loop_(loop), media_session_(media_session), id_(-1) {}

RtspSession::~RtspSession() {
    states_.clear();
    rtp_conn_.reset();
    rtcp_conn_.reset();
    media_session_.reset();
}

void RtspSession::Setup(const std::string &track,
                        const muduo::net::InetAddress &peer_rtp_addr,
                        const muduo::net::InetAddress &peer_rtcp_addr,
                        unsigned short &local_rtp_port,
                        unsigned short &local_rtcp_port) {

    std::random_device rd;

    for (;;) {
        local_rtp_port = rd() & 0xfffe;

        // rtp
        {
            LOG_DEBUG << "try to bind rtp port " << local_rtp_port;

            int rtp_sockfd = muduo::net::sockets::CreateNonblockingUdp(AF_INET);
            muduo::net::InetAddress local_rtp_addr(local_rtp_port);
            rtp_conn_.reset(new muduo::net::UdpVirtualConnection(
                loop_, "rtp_conn", rtp_sockfd, local_rtp_addr, peer_rtp_addr));

            if (!rtp_conn_->Bind()) {
                LOG_ERROR << "failed to bind rtp " << local_rtp_addr.IpPort();
                continue;
            }
        }

        // rtcp
        {
            local_rtcp_port = local_rtp_port + 1;
            LOG_DEBUG << "try to bind rtcp port " << local_rtcp_port;
            int rtcp_sockfd =
                muduo::net::sockets::CreateNonblockingUdp(AF_INET);

            muduo::net::InetAddress local_rtcp_addr(local_rtcp_port);

            rtcp_conn_.reset(new muduo::net::UdpVirtualConnection(
                loop_, "rtcp_conn", rtcp_sockfd, local_rtcp_addr,
                peer_rtcp_addr));

            if (!rtcp_conn_->Bind()) {
                LOG_ERROR << "failed to bind rtcp " << local_rtcp_addr.IpPort();
                continue;
            }
        }

        rtp_conn_->BindingFinished();
        rtcp_conn_->BindingFinished();

        id_ = rd() & 0xffffff;
        break;
    }

    auto valid_media_session = media_session_.lock();

    MediaSubsessionPtr subsession = valid_media_session->GetSubsession(track);

    RtpSinkPtr rtp_sink = subsession->NewRtpSink(rtp_conn_);
    MultiFrameSourcePtr frame_source = subsession->NewMultiFrameSouce();

    StreamStatePtr state = std::make_shared<RtspStreamState>(
        loop_, subsession, rtp_sink, frame_source);

    states_.insert(std::make_pair(track, state));
}

void RtspSession::Play() {
    for (auto &&i : states_) {
        i.second->Play();
    }
}

void RtspSession::Teardown() {}

} // namespace muduo_media