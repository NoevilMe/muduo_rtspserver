#ifndef B932C30B_8468_4B16_95D6_B7C4EF8DE5C0
#define B932C30B_8468_4B16_95D6_B7C4EF8DE5C0

#include "eventloop/event_loop.h"
#include "net/tcp_server.h"
#include "rtsp_connection.h"

class RtspServer {
public:
    RtspServer(muduo::event_loop::EventLoop *loop,
               const muduo::net::InetAddress &listen_addr,
               const std::string &name, bool reuse_port = false);
    ~RtspServer();

    void Start();

private:
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);

private:
    muduo::net::TcpServer tcp_server_;

    std::map<std::string, RtspConnectionPtr> connections_;
};

#endif /* B932C30B_8468_4B16_95D6_B7C4EF8DE5C0 */
