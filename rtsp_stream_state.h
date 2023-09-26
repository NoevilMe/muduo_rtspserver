#ifndef ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7
#define ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7

#include "media/media_subsession.h"
#include "media/rtp_sink.h"
#include "net/udp_virtual_connection.h"
#include "stream_state.h"

namespace muduo_media {

using RtcpGoodbyeCallback = std::function<void(uint32_t)>;

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

    void set_rtcp_goodbye_callback(const RtcpGoodbyeCallback &cb) {
        goodbye_cb_ = cb;
    }

    void OnUdpRtcpMessage(const muduo::net::UdpServerPtr &,
                          muduo::net::Buffer *, struct sockaddr_in6 *,
                          muduo::event_loop::Timestamp);

private:
    void PlayOnce();

    void OnRtcpRR(uint8_t rc, const char *buf, size_t size);
    void OnRtcpSDES(uint8_t rc, const char *buf, size_t size);

private:
    MediaSubsessionPtr media_subsession_;
    RtpSinkPtr rtp_sink_;
    MultiFrameSourcePtr frame_source_;

    RtcpGoodbyeCallback goodbye_cb_;
};

using RtspStreamStatePtr = std::shared_ptr<RtspStreamState>;

} // namespace muduo_media

#endif /* ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7 */
