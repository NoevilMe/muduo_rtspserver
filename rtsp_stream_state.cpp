#include "rtsp_stream_state.h"
#include "logger/logger.h"
#include "media/av_packet.h"
#include "rtp.h"

namespace muduo_media {

RtspStreamState::RtspStreamState(muduo::event_loop::EventLoop *loop,
                                 const MediaSubsessionPtr &media_subsession,
                                 const RtpSinkPtr &rtp_sink,
                                 const MultiFrameSourcePtr &frame_source)
    : StreamState(loop),
      media_subsession_(media_subsession),
      rtp_sink_(rtp_sink),
      frame_source_(frame_source) {}

RtspStreamState::~RtspStreamState() {}

void RtspStreamState::Play() {
    loop_->QueueInLoop(std::bind(&RtspStreamState::PlayOnce, this));
}

void RtspStreamState::PlayOnce() {
    AVPacket frame_packet;
    // 读取一帧, 获取NALU
    // 同一帧可能有多个NALU，如果是同一个帧的多个NALU则需要立即发送
    if (!frame_source_->GetNextFrame(&frame_packet)) {
        LOG_ERROR << "Frame source get next frame fail";
    }

    LOG_DEBUG << "data length " << frame_packet.size;
    ++play_frames_;

    ++play_packets_;
    //  打包成RTP并发送

    std::shared_ptr<void> rtp_header = std::make_shared<RtpHeader>();

    rtp_sink_->Send(frame_packet.buffer.get(), frame_packet.size, rtp_header);

    // 计划下一次发送
    if (frame_packet.size > 0) {
        loop_->RunAfter(0.04, std::bind(&RtspStreamState::PlayOnce, this));
    }
}

} // namespace muduo_media