#ifndef D4439448_0C51_428E_913A_3171964D0153
#define D4439448_0C51_428E_913A_3171964D0153

#include "eventloop/event_loop.h"
#include "net/callback.h"
#include "rtsp_message.h"

namespace rtsp {

class MediaSession;

using GetMediaSessionCallback =
    std::function<std::shared_ptr<MediaSession>(const std::string &)>;

class RtspConnection {
public:
    RtspConnection(const muduo::net::TcpConnectionPtr &conn);
    ~RtspConnection() = default;

    void set_get_media_session_callback(const GetMediaSessionCallback &cb) {
        get_media_session_callback_ = cb;
    }

    void OnMessage(const muduo::net::TcpConnectionPtr conn,
                   muduo::net::Buffer *buf,
                   muduo::event_loop::Timestamp timestamp);

private:
    std::shared_ptr<RtspRequestHead>
    ParseRequestHeader(muduo::net::Buffer *buf);

    bool ParseCSeq(muduo::net::Buffer *buf, int &seq);

    void DiscardAllData(muduo::net::Buffer *buf);

    void HandleRequestMethodOptions(
        muduo::net::Buffer *buf,
        const std::shared_ptr<RtspRequestHead> &header);

    void HandleRequestMethodDescribe(
        muduo::net::Buffer *buf,
        const std::shared_ptr<RtspRequestHead> &header);

    void
    HandleRequestMethodSetup(muduo::net::Buffer *buf,
                             const std::shared_ptr<RtspRequestHead> &header);

    std::string ShortResponseMessage(const std::string &version,
                                     RtspStatusCode code, int cseq);

    void SendShortResponse(const std::string &version, RtspStatusCode code,
                           int cseq);

    void SendShortResponse(const RtspResponseHead &resp_header);

private:
    const muduo::net::TcpConnectionPtr tcp_conn_;

    std::weak_ptr<MediaSession> active_media_session_;

    GetMediaSessionCallback get_media_session_callback_;
};

using RtspConnectionPtr = std::shared_ptr<RtspConnection>;

} // namespace rtsp

#endif /* D4439448_0C51_428E_913A_3171964D0153 */
