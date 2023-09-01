#ifndef D4439448_0C51_428E_913A_3171964D0153
#define D4439448_0C51_428E_913A_3171964D0153

#include "eventloop/event_loop.h"
#include "net/callback.h"

struct RtspUrl {
    std::string entire;
    std::string prefix;
    std::string host;
    int port = 0;
    std::string session; // suffix
};

struct RtspRequestHeader {
    std::string method;
    std::string version;
    int cseq = 0;
    RtspUrl url;
};

class RtspConnection {
public:
    RtspConnection(const muduo::net::TcpConnectionPtr &conn);
    ~RtspConnection() = default;

private:
    void OnMessage(const muduo::net::TcpConnectionPtr conn,
                   muduo::net::Buffer *buf,
                   muduo::event_loop::Timestamp timestamp);

    std::shared_ptr<RtspRequestHeader>
    ParseRequestHeader(muduo::net::Buffer *buf);

    bool ParseCSeq(muduo::net::Buffer *buf, int &seq);

    void DiscardAllData(muduo::net::Buffer *buf);

private:
    const muduo::net::TcpConnectionPtr tcp_conn_;
};

using RtspConnectionPtr = std::shared_ptr<RtspConnection>;

#endif /* D4439448_0C51_428E_913A_3171964D0153 */
