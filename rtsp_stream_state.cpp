#include "rtsp_stream_state.h"
#include "eventloop/endian.h"
#include "logger/logger.h"
#include "media/av_packet.h"
#include "media/rtp.h"

#include <random>

namespace muduo_media {

RtspStreamState::RtspStreamState(muduo::event_loop::EventLoop *loop,
                                 const MediaSubsessionPtr &media_subsession,
                                 const RtpSinkPtr &rtp_sink,
                                 const MultiFrameSourcePtr &frame_source)
    : StreamState(loop),
      media_subsession_(media_subsession),
      rtp_sink_(rtp_sink),
      frame_source_(frame_source) {
    std::random_device rd;

    init_seq_ = rd() & 0xFF; // limited
    ssrc_ = rd();
}

RtspStreamState::~RtspStreamState() {
    frame_source_.reset();
    media_subsession_.reset();
    rtp_sink_.reset();
}

void RtspStreamState::Play() {
    loop_->QueueInLoop(std::bind(&RtspStreamState::PlayOnce, this));
}

void RtspStreamState::PlayOnce() {

    // In timer, this object may not exist
    if (!frame_source_) {
        return;
    }

    AVPacket frame_packet;
    // 读取一帧, 获取NALU
    // 同一帧可能有多个NALU，如果是同一个帧的多个NALU则需要立即发送
    if (!frame_source_->GetNextFrame(&frame_packet)) {
        LOG_ERROR << "frame source get next frame fail";
    } else {
        LOG_DEBUG << "data length " << frame_packet.size;
        ++play_frames_;

        ++play_packets_;
        //  打包成RTP并发送

        std::shared_ptr<RtpHeader> rtp_header = std::make_shared<RtpHeader>();
        ::bzero(rtp_header.get(), sizeof(RtpHeader));
        rtp_header->version = RTP_VESION;
        rtp_header->padding = 0;
        rtp_header->extension = 0;
        rtp_header->csrcLen = 0;
        rtp_header->marker = 1; //  最后一帧
        rtp_header->payloadType = RTP_PAYLOAD_TYPE_H264;
        rtp_header->seq =
            muduo::HostToNetwork16(init_seq_++); // 随机初值，自动增长 htons()
        rtp_header->timestamp = muduo::HostToNetwork32(
            frame_packet.timestamp); //需要根据源计算 htonl()
        rtp_header->ssrc = muduo::HostToNetwork32(ssrc_); // 信号源id

        rtp_sink_->Send(frame_packet.buffer.get(), frame_packet.size,
                        rtp_header);

        // 计划下一次发送
        if (frame_packet.size > 0) {
            loop_->RunAfter(0.04, std::bind(&RtspStreamState::PlayOnce, this));
        }
    }
}

} // namespace muduo_media