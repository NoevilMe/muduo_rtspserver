#ifndef D4439448_0C51_428E_913A_3171964D0153
#define D4439448_0C51_428E_913A_3171964D0153

#include "eventloop/event_loop.h"
#include "net/callback.h"
#include "rtsp_message.h"
#include "rtsp_type.h"

namespace muduo_media {

class MediaSession;
class RtspSession;

using GetMediaSessionCallback =
    std::function<std::shared_ptr<MediaSession>(const std::string &)>;

class RtspConnection {
public:
    RtspConnection(const muduo::net::TcpConnectionPtr &conn,
                   const GetMediaSessionCallback &cb);
    ~RtspConnection();

    void OnMessage(const muduo::net::TcpConnectionPtr conn,
                   muduo::net::Buffer *buf,
                   muduo::event_loop::Timestamp timestamp);

    // 下一条消息解析类型
    enum NextMessageType { kMessageNone, kMessageILFrame };

    struct InterleavedFrameInfo {
        uint8_t channel;
        uint16_t length;

        void Reset() {
            this->channel = 0;
            this->length = 0;
        }
    };

private:
    std::shared_ptr<RtspRequestHead>
    ParseRequestHead(muduo::net::Buffer *buf,
                     std::vector<std::string> &gap_lines);

    void DiscardAllData(muduo::net::Buffer *buf);

    void HandleMethodOptions(muduo::net::Buffer *buf,
                             const std::shared_ptr<RtspRequestHead> &head);

    void HandleMethodDescribe(muduo::net::Buffer *buf,
                              const std::shared_ptr<RtspRequestHead> &head,
                              const std::vector<std::string> &gap_lines);

    void HandleMethodSetup(muduo::net::Buffer *buf,
                           const std::shared_ptr<RtspRequestHead> &head,
                           const std::vector<std::string> &gap_lines);

    void HandleMethodPlay(muduo::net::Buffer *buf,
                          const std::shared_ptr<RtspRequestHead> &head);

    void HandleMethodTeardown(muduo::net::Buffer *buf,
                              const std::shared_ptr<RtspRequestHead> &head);

    std::string ShortResponseMessage(const std::string &version,
                                     RtspStatusCode code, int cseq);

    void SendShortResponse(const std::string &version, RtspStatusCode code,
                           int cseq);

    void SendShortResponse(const RtspResponseHead &resp_header);

    void SendResponse(const char *buf, int size);

    // Parse
    void ParseInterleavedFrameHead(const char *buf, size_t size);

private:
    muduo::net::TcpConnectionPtr tcp_conn_;
    GetMediaSessionCallback get_media_session_callback_;
    NextMessageType next_type_;
    InterleavedFrameInfo next_ilframe_;

    std::weak_ptr<MediaSession> active_media_session_;
    std::string media_session_name_;

    std::shared_ptr<RtspSession> rtsp_session_;

    RtpTransportProtocol rtp_transport_;
};

using RtspConnectionPtr = std::shared_ptr<RtspConnection>;

} // namespace muduo_media

#endif /* D4439448_0C51_428E_913A_3171964D0153 */
