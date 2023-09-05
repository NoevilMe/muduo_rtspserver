
#include "rtsp_server.h"
#include "logger/logger.h"
#include "net/inet_address.h"
#include "net/tcp_connection.h"

namespace rtsp {

RtspServer::RtspServer(muduo::event_loop::EventLoop *loop,
                       const muduo::net::InetAddress &listen_addr,
                       const std::string &name, bool reuse_port)
    : tcp_server_(loop, listen_addr, name, reuse_port) {

    // 连接已经建立，但是还没开始读取数据
    tcp_server_.set_before_reading_callback(
        std::bind(&RtspServer::OnBeforeReading, this, std::placeholders::_1));

    //  设置连接回调，用于管理连接。
    //  connection callback被调用的时候，已经开始接受数据了。
    //  所以不能在这之后再设置message callback
    tcp_server_.set_connection_callback(
        std::bind(&RtspServer::OnConnection, this, std::placeholders::_1));
}

RtspServer::~RtspServer() {}

void RtspServer::Start() { tcp_server_.Start(); }

void RtspServer::AddMediaSession(const MediaSessionPtr &session) {
    sessions_.insert(std::make_pair(session->path(), session));
    LOG_INFO << "added session " << session->path();
}

void RtspServer::OnBeforeReading(const muduo::net::TcpConnectionPtr &conn) {
    assert(conn->Connected());

    LOG_INFO << "connected " << conn->peer_addr().IpPort();
    RtspConnectionPtr rtsp_conn(new RtspConnection(conn));

    rtsp_conn->set_get_media_session_callback(
        std::bind(&RtspServer::OnGetMediaSession, this, std::placeholders::_1));
    connections_[conn->name()] = rtsp_conn;
}

void RtspServer::OnConnection(const muduo::net::TcpConnectionPtr &conn) {
    if (conn->Connected()) {
        LOG_INFO << "start reading data from " << conn->peer_addr().IpPort();
    } else {
        LOG_INFO << "disconnected " << conn->peer_addr().IpPort();
        connections_.erase(conn->name());
    }
}

MediaSessionPtr RtspServer::OnGetMediaSession(const std::string &path) {
    auto it = sessions_.find(path);
    if (it == sessions_.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

} // namespace rtsp
