#include "rtsp_session.h"
#include "eventloop/endian.h"
#include "logger/logger.h"
#include "media/rtcp.h"
#include "net/tcp_connection.h"
#include "rtsp_stream_state.h"

#include <random>

namespace muduo_media {
RtspSession::RtspSession(muduo::event_loop::EventLoop *loop,
                         const std::weak_ptr<MediaSession> &media_session)
    : loop_(loop), media_session_(media_session), id_(-1) {

    LOG_DEBUG << "RtspSession::ctor at " << this;
}

RtspSession::~RtspSession() {
    LOG_DEBUG << "RtspSession::dtor at " << this;

    states_.clear();
    media_session_.reset();
    tcp_conn_.reset();
}

void RtspSession::Setup(const std::string &track,
                        const muduo::net::InetAddress &peer_rtp_addr,
                        const muduo::net::InetAddress &peer_rtcp_addr,
                        unsigned short &local_rtp_port,
                        unsigned short &local_rtcp_port) {

    std::shared_ptr<muduo::net::UdpVirtualConnection> rtp_conn;
    std::shared_ptr<muduo::net::UdpVirtualConnection> rtcp_conn;
    std::random_device rd;

    for (;;) {
        local_rtp_port = rd() & 0xfffe;

        // rtp
        {
            LOG_DEBUG << "try to bind rtp port " << local_rtp_port;

            int rtp_sockfd = muduo::net::sockets::CreateNonblockingUdp(AF_INET);
            muduo::net::InetAddress local_rtp_addr(local_rtp_port);
            rtp_conn.reset(new muduo::net::UdpVirtualConnection(
                loop_, "rtp_conn", rtp_sockfd, local_rtp_addr, peer_rtp_addr));

            if (!rtp_conn->Bind()) {
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

            rtcp_conn.reset(new muduo::net::UdpVirtualConnection(
                loop_, "rtcp_conn", rtcp_sockfd, local_rtcp_addr,
                peer_rtcp_addr));

            if (!rtcp_conn->Bind()) {
                LOG_ERROR << "failed to bind rtcp " << local_rtcp_addr.IpPort();
                continue;
            }
        }

        rtp_conn->BindingFinished();
        rtcp_conn->BindingFinished();

        id_ = rd() & 0xffffff;
        break;
    }

    auto valid_media_session = media_session_.lock();

    MediaSubsessionPtr subsession = valid_media_session->GetSubsession(track);

    RtpSinkPtr rtp_sink = subsession->NewRtpSink(rtp_conn);
    MultiFrameSourcePtr frame_source = subsession->NewMultiFrameSouce();

    RtspStreamStatePtr state = std::make_shared<RtspStreamState>(
        loop_, subsession, rtp_sink, frame_source);

    rtcp_conn->set_message_callback(std::bind(
        &RtspStreamState::OnUdpRtcpMessage, state.get(), std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    // state holds the rtcp_conn by function object
    state->set_send_rtcp_message_callback(
        std::bind(&RtspSession::SendUdpRtcpMessages, this, rtcp_conn,
                  std::placeholders::_1));

    states_.insert(std::make_pair(track, state));

    {
        std::shared_ptr<ChannelOrPortStreamBinding> rtp_binding =
            std::make_shared<ChannelOrPortStreamBinding>();
        rtp_binding->cop_type = kPortRtp;
        rtp_binding->cop_rtp = local_rtp_port;
        rtp_binding->cop_rtcp = local_rtcp_port;
        rtp_binding->media_subsession = track;
        rtp_binding->state = state;

        binding_states_.insert(std::make_pair(local_rtp_port, rtp_binding));
    }

    {
        std::shared_ptr<ChannelOrPortStreamBinding> rtcp_binding =
            std::make_shared<ChannelOrPortStreamBinding>();
        rtcp_binding->cop_type = kPortRtcp;
        rtcp_binding->cop_rtp = local_rtp_port;
        rtcp_binding->cop_rtcp = local_rtcp_port;
        rtcp_binding->media_subsession = track;
        rtcp_binding->state = state;
        binding_states_.insert(std::make_pair(local_rtcp_port, rtcp_binding));
    }
}

void RtspSession::Setup(const std::string &track,
                        const muduo::net::TcpConnectionPtr &tcp_conn,
                        int8_t rtp_channel, int8_t rtcp_channel) {

    tcp_conn_ = tcp_conn;

    std::random_device rd;
    id_ = rd() & 0xffffff;
    auto valid_media_session = media_session_.lock();

    MediaSubsessionPtr subsession = valid_media_session->GetSubsession(track);

    RtpSinkPtr rtp_sink = subsession->NewRtpSink(tcp_conn, rtp_channel);
    MultiFrameSourcePtr frame_source = subsession->NewMultiFrameSouce();

    RtspStreamStatePtr state = std::make_shared<RtspStreamState>(
        loop_, subsession, rtp_sink, frame_source);
    state->set_send_rtcp_message_callback(
        std::bind(&RtspSession::SendTcpRtcpMessages, this, rtcp_channel,
                  std::placeholders::_1));

    states_.insert(std::make_pair(track, state));

    {
        std::shared_ptr<ChannelOrPortStreamBinding> rtp_binding =
            std::make_shared<ChannelOrPortStreamBinding>();
        rtp_binding->cop_type = kChannelRtp;
        rtp_binding->cop_rtp = rtp_channel;
        rtp_binding->cop_rtcp = rtcp_channel;
        rtp_binding->media_subsession = track;
        rtp_binding->state = state;

        binding_states_.insert(std::make_pair(rtp_channel, rtp_binding));
    }

    {
        std::shared_ptr<ChannelOrPortStreamBinding> rtcp_binding =
            std::make_shared<ChannelOrPortStreamBinding>();
        rtcp_binding->cop_type = kChannelRtcp;
        rtcp_binding->cop_rtp = rtp_channel;
        rtcp_binding->cop_rtcp = rtcp_channel;
        rtcp_binding->media_subsession = track;
        rtcp_binding->state = state;
        binding_states_.insert(std::make_pair(rtcp_channel, rtcp_binding));
    }
}

void RtspSession::Play() {
    for (auto &&i : states_) {
        i.second->Play();
    }
}

void RtspSession::Teardown() {
    for (auto &&i : states_) {
        i.second->Teardown();
    }
    // TODO: release session
}

void RtspSession::ParseTcpInterleavedFrameBody(uint8_t channel, const char *buf,
                                               size_t size) {

    auto it = binding_states_.find(channel);
    if (it == binding_states_.end()) {
        LOG_ERROR << "can't find channel " << channel << " binding";
        return;
    }

    if (it->second->cop_type == kChannelRtcp ||
        it->second->cop_type == kPortRtcp) {
        it->second->state->ParseRTCP(buf, size);
    } else {
        it->second->state->ParseRTP(buf, size);
    }
}

void RtspSession::SendTcpRtcpMessages(
    uint8_t channel, const std::vector<std::shared_ptr<RtcpMessage>> &msg) {
    LOG_DEBUG << "send RTCP on channel " << channel;

    uint8_t ptr[4] = {0};
    ptr[0] = '$';
    ptr[1] = (uint8_t)channel;

    std::string binary;
    for (auto &m : msg) {
        binary.append(m->Serialize());
    }

    ptr[2] = (uint8_t)((binary.size() & 0xFF00) >> 8);
    ptr[3] = (uint8_t)(binary.size() & 0xFF);

    tcp_conn_->Send(ptr, 4);
    tcp_conn_->Send(binary);
}

void RtspSession::SendUdpRtcpMessages(
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn,
    const RtcpMessageVector &msg) {
    LOG_DEBUG << "send RTCP on udp " << udp_conn->name();

    std::string binary;
    for (auto &m : msg) {
        binary.append(m->Serialize());
    }

    // FIXME: too long
    udp_conn->Send(binary);
}

} // namespace muduo_media