#include "rtp_connection.h"
#include "net/inet_socket.h"

#include <random>

namespace muduo_media {
RtpUdpConnection::RtpUdpConnection(unsigned short peer_rtp_port,
                                   unsigned short peer_rtcp_port)
    : peer_rtp_port_(peer_rtp_port), peer_rtcp_port_(peer_rtcp_port) {}

RtpUdpConnection::~RtpUdpConnection() {}

bool RtpUdpConnection::Bind(unsigned short &local_rtp_port,
                            unsigned short &local_rtcp_port) {

    // std::random_device rd;
    // _localRtpPort[channelId] = rd() & 0xfffe;
    // _localRtcpPort[channelId] = _localRtpPort[channelId] + 1;

    // int rtp_fd_ = muduo::net::sockets::CreateNonblockingDdp(AF_INET);
    // rtcp_fd_ = muduo::net::sockets::CreateNonblockingDdp(AF_INET);

    // struct sockaddr_in rtpAddr = {0};
    // rtpAddr.sin_family = AF_INET;
    // rtpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // rtpAddr.sin_port = htons(_localRtpPort[channelId]);
    // if (bind(_rtpfd[channelId], (struct sockaddr *)&rtpAddr, sizeof rtpAddr) ==
    //     -1) {
    //     close(_rtpfd[channelId]);
    //     continue;
    // }

    // _rtcpfd[channelId] = ::socket(AF_INET, SOCK_DGRAM, 0);
    // struct sockaddr_in rtcpAddr = {0};
    // rtcpAddr.sin_family = AF_INET;
    // rtcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // rtcpAddr.sin_port = htons(_localRtcpPort[channelId]);
    // if (bind(_rtcpfd[channelId], (struct sockaddr *)&rtcpAddr,
    //          sizeof rtcpAddr) == -1) {
    //     close(_rtcpfd[channelId]);
    //     close(_rtpfd[channelId]);
    //     continue;
    // }
    return true;
}

RtpTcpConnection::RtpTcpConnection() {}

RtpTcpConnection::~RtpTcpConnection() {}

} // namespace rtsp