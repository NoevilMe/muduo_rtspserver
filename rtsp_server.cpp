
#include "rtsp_server.h"
#include "logger/logger.h"
#include "net/inet_address.h"
#include "net/tcp_connection.h"

RtspServer::RtspServer(muduo::event_loop::EventLoop *loop,
                       const muduo::net::InetAddress &listen_addr,
                       const std::string &name, bool reuse_port)
    : tcp_server_(loop, listen_addr, name, reuse_port) {

    //   设置连接回调，用于管理连接
    tcp_server_.set_connection_callback(
        std::bind(&RtspServer::OnConnection, this, std::placeholders::_1));
}

RtspServer::~RtspServer() {}

void RtspServer::Start() { tcp_server_.Start(); }

void RtspServer::OnConnection(const muduo::net::TcpConnectionPtr &conn) {

    if (conn->Connected()) {
        LOG_INFO << "connected " << conn->peer_addr().IpPort();
        RtspConnectionPtr rtsp_conn(new RtspConnection(conn));
        connections_[conn->name()] = rtsp_conn;
    } else {
        LOG_INFO << "disconnected " << conn->peer_addr().IpPort();
        connections_.erase(conn->name());
    }
}
