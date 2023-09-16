#ifndef D4439448_0C51_428E_913A_3171964D0153
#define D4439448_0C51_428E_913A_3171964D0153

#include "eventloop/event_loop.h"
#include "net/callback.h"
#include "rtsp_message.h"

namespace muduo_media {

class MediaSession;
class RtspSession;

using GetMediaSessionCallback =
    std::function<std::shared_ptr<MediaSession>(const std::string &)>;

class RtspConnection {
public:
    RtspConnection(const muduo::net::TcpConnectionPtr &conn,
                   const GetMediaSessionCallback &cb);
    ~RtspConnection() = default;

    void OnMessage(const muduo::net::TcpConnectionPtr conn,
                   muduo::net::Buffer *buf,
                   muduo::event_loop::Timestamp timestamp);

private:
    std::shared_ptr<RtspRequestHead> ParseRequestHead(muduo::net::Buffer *buf);

    bool ParseCSeq(muduo::net::Buffer *buf, int &seq);

    void DiscardAllData(muduo::net::Buffer *buf);

    void HandleMethodOptions(muduo::net::Buffer *buf,
                             const std::shared_ptr<RtspRequestHead> &header);

    void HandleMethodDescribe(muduo::net::Buffer *buf,
                              const std::shared_ptr<RtspRequestHead> &header);

    void HandleMethodSetup(muduo::net::Buffer *buf,
                           const std::shared_ptr<RtspRequestHead> &header);

    void HandleMethodPlay(muduo::net::Buffer *buf,
                          const std::shared_ptr<RtspRequestHead> &header);

    void HandleMethodTeardown(muduo::net::Buffer *buf,
                              const std::shared_ptr<RtspRequestHead> &header);

    std::string ShortResponseMessage(const std::string &version,
                                     RtspStatusCode code, int cseq);

    void SendShortResponse(const std::string &version, RtspStatusCode code,
                           int cseq);

    void SendShortResponse(const RtspResponseHead &resp_header);

    void SendResponse(const char *buf, int size);

private:
    const muduo::net::TcpConnectionPtr tcp_conn_;
    GetMediaSessionCallback get_media_session_callback_;

    std::weak_ptr<MediaSession> active_media_session_;
    std::string media_session_name_;

    std::shared_ptr<RtspSession> rtsp_session_;
};

using RtspConnectionPtr = std::shared_ptr<RtspConnection>;

} // namespace muduo_media

#endif /* D4439448_0C51_428E_913A_3171964D0153 */
