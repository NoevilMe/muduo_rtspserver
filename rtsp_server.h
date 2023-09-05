#ifndef B932C30B_8468_4B16_95D6_B7C4EF8DE5C0
#define B932C30B_8468_4B16_95D6_B7C4EF8DE5C0

#include "eventloop/event_loop.h"
#include "media_session.h"
#include "net/tcp_server.h"
#include "rtsp_connection.h"

namespace rtsp {

class RtspServer {
public:
    RtspServer(muduo::event_loop::EventLoop *loop,
               const muduo::net::InetAddress &listen_addr,
               const std::string &name, bool reuse_port = false);
    ~RtspServer();

    void Start();

    void AddMediaSession(const MediaSessionPtr &session);

private:
    void OnBeforeReading(const muduo::net::TcpConnectionPtr &conn);
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);

    MediaSessionPtr OnGetMediaSession(const std::string &path);

private:
    muduo::net::TcpServer tcp_server_;

    std::map<std::string, RtspConnectionPtr> connections_;
    std::map<std::string, MediaSessionPtr> sessions_;
};

} // namespace rtsp

#endif /* B932C30B_8468_4B16_95D6_B7C4EF8DE5C0 */
