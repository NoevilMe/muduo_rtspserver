#ifndef ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7
#define ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7

#include "media/media_subsession.h"
#include "media/rtcp.h"
#include "media/rtp_sink.h"
#include "net/udp_virtual_connection.h"
#include "stream_state.h"

namespace muduo_media {

using SendRtcpMessageCallback =
    std::function<void(const std::vector<std::shared_ptr<RtcpMessage>> &)>;

/// @brief RtspSession中表示当前流的状态
class RtspStreamState : public StreamState {
public:
    RtspStreamState(muduo::event_loop::EventLoop *loop,
                    const MediaSubsessionPtr &media_subsession,
                    const RtpSinkPtr &rtp_sink,
                    const MultiFrameSourcePtr &frame_source);
    ~RtspStreamState();

    virtual void Play() override;
    virtual void Teardown() override;

    virtual void ParseRTP(const char *buf, size_t size) override;
    virtual void ParseRTCP(const char *buf, size_t size) override;

    void set_send_rtcp_message_callback(const SendRtcpMessageCallback &cb) {
        rtcp_cb_ = cb;
    }

    void OnUdpRtcpMessage(const muduo::net::UdpServerPtr &,
                          muduo::net::Buffer *, struct sockaddr_in6 *,
                          muduo::event_loop::Timestamp);

private:
    void PlayOnce(bool update_ts);

    void SendRtcpBye();

private:
    MediaSubsessionPtr media_subsession_;
    RtpSinkPtr rtp_sink_;
    MultiFrameSourcePtr frame_source_;

    SendRtcpMessageCallback rtcp_cb_;

    uint32_t last_rtp_ts_;
    uint32_t ts_duration_;
    double play_interval_;
};

using RtspStreamStatePtr = std::shared_ptr<RtspStreamState>;

} // namespace muduo_media

#endif /* ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7 */
