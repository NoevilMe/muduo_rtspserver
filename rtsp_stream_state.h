#ifndef ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7
#define ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7

#include "media/media_subsession.h"
#include "media/rtp_sink.h"
#include "stream_state.h"

namespace muduo_media {

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

private:
    void PlayOnce();

private:
    MediaSubsessionPtr media_subsession_;
    RtpSinkPtr rtp_sink_;
    MultiFrameSourcePtr frame_source_;

    uint16_t init_seq_;
    uint32_t ssrc_;
};

using RtspStreamStatePtr = std::shared_ptr<RtspStreamState>;

} // namespace muduo_media

#endif /* ABB750C3_2A77_4AC5_811A_AD81F9C0B3F7 */
