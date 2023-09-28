#include "rtsp_stream_state.h"
#include "eventloop/endian.h"
#include "logger/logger.h"
#include "media/av_packet.h"
#include "media/defs.h"
#include "media/rtcp.h"
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
      frame_source_(frame_source),
      last_rtp_ts_(0),
      play_interval_(0.0) {

    LOG_DEBUG << "RtspStreamState::ctor at " << this;
}

RtspStreamState::~RtspStreamState() {
    LOG_DEBUG << "RtspStreamState::dtor at " << this;
    // reset members, they could be used in timer function object
    frame_source_.reset();
    media_subsession_.reset();
    rtp_sink_.reset();
    playing_ = false;
}

void RtspStreamState::Play() {
    playing_ = true;
    try {
        ts_duration_ = media_subsession_->Duration();
        play_interval_ = 1 / media_subsession_->fps();
    } catch (...) {
        ts_duration_ = defs::kMediaTsDuration;
        play_interval_ = 0.04;
    }

    loop_->QueueInLoop(std::bind(&RtspStreamState::PlayOnce, this, true));
}

void RtspStreamState::Teardown() { playing_ = false; }

void RtspStreamState::ParseRTP(const char *buf, size_t size) {}

void RtspStreamState::ParseRTCP(const char *buf, size_t size) {
    size_t parse_size = 0;
    size_t left_size = size;
    while (left_size > sizeof(RtcpHeader)) {
        RtcpHeader rtcp_header = {0};
        memcpy(&rtcp_header, buf + parse_size, sizeof(RtcpHeader));
        rtcp_header.length = muduo::NetworkToHost16(rtcp_header.length);
        rtcp_header.ssrc = muduo::NetworkToHost32(rtcp_header.ssrc);

        size_t packet_size = (rtcp_header.length + 1) * RTCP_LENGTH_DWORD;

        LOG_DEBUG << "RTCP V " << rtcp_header.v << ", P " << rtcp_header.p
                  << ", RC " << rtcp_header.rc << ", PT " << rtcp_header.pt
                  << ", length " << rtcp_header.length << ", ssrc "
                  << rtcp_header.ssrc << ", RTCP packet size " << packet_size;

        if (rtcp_header.pt == (uint8_t)RtcpPacketType::RTCP_RR) {
            std::unique_ptr<RtcpRRMessage> rtcp_rr(new RtcpRRMessage);
            rtcp_rr->header = rtcp_header;
            rtcp_rr->Deserialize(buf + parse_size + sizeof(RtcpHeader),
                                 (rtcp_header.length - 1) * RTCP_LENGTH_DWORD);

        } else if (rtcp_header.pt == (uint8_t)RtcpPacketType::RTCP_SDES) {
            std::unique_ptr<RtcpSDESMessage> rtcp_sdes(new RtcpSDESMessage);
            rtcp_sdes->header = rtcp_header;
            rtcp_sdes->Deserialize(buf + parse_size + sizeof(RtcpHeader) -
                                       sizeof(rtcp_header.ssrc),
                                   rtcp_header.length * RTCP_LENGTH_DWORD);
        } else if (rtcp_header.pt == (uint8_t)RtcpPacketType::RTCP_BYE) {
            std::unique_ptr<RtcpBYEMessage> rtcp_sdes(new RtcpBYEMessage);
            rtcp_sdes->header = rtcp_header;
            rtcp_sdes->Deserialize(buf + parse_size + sizeof(RtcpHeader),
                                   (rtcp_header.length - 1) *
                                       RTCP_LENGTH_DWORD);
        }

        parse_size += packet_size;
        left_size -= packet_size;
        LOG_TRACE << "left size " << left_size;
    }
}

void RtspStreamState::OnUdpRtcpMessage(const muduo::net::UdpServerPtr &,
                                       muduo::net::Buffer *buf,
                                       struct sockaddr_in6 *addr,
                                       muduo::event_loop::Timestamp timestamp) {

    ParseRTCP(buf->Peek(), buf->ReadableBytes());

    buf->RetrieveAll();
}

void RtspStreamState::SendRtcpBye() {
    if (rtcp_cb_) {
        std::vector<std::shared_ptr<RtcpMessage>> msgs;

        std::shared_ptr<RtcpSRMessage> sr = std::make_shared<RtcpSRMessage>();
        sr->header.ssrc = frame_source_->ssrc();
        auto &sender_info = sr->sender_info;

        auto ts = muduo::event_loop::Timestamp::TimespecNow();
        sender_info.ts_msw =
            ts.tv_sec + 0x83AA7E80; // NTP timestamp most-significant word (1970
                                    // epoch -> 1900 epoch)
        // 1 second = 1,000,000,000,000 picoseconds
        /* Convert nanoseconds to 32-bits fraction (232 picosecond units) */
        sender_info.ts_lsw =
            (uint32_t)((uint64_t)ts.tv_nsec * ((uint64_t)1 << 32) / 1000000000);

        sender_info.rtp_ts = last_rtp_ts_;
        sender_info.octets = rtp_sink_->octets();
        sender_info.packets = rtp_sink_->packets();

        msgs.push_back(sr);

        std::shared_ptr<RtcpBYEMessage> bye =
            std::make_shared<RtcpBYEMessage>();
        bye->header.ssrc = frame_source_->ssrc();

        msgs.push_back(bye);
        rtcp_cb_(msgs);
    }
}

void RtspStreamState::PlayOnce(bool update_ts) {
    if (!playing_) {
        LOG_DEBUG << "not playing at " << this;
        return;
    }

    // IMPORTANT!!! In timer, this object might be released.
    if (!frame_source_) {
        LOG_WARN << "PlayOnce cancel at " << this;
        return;
    }

    if (0 == last_rtp_ts_) {
        std::random_device rd;
        last_rtp_ts_ = rd() & 0xffffff;
    }

    AVPacket frame_packet;
    // 读取一帧, 获取NALU
    // 同一帧可能有多个NALU，如果是同一个帧的多个NALU则需要立即发送
    if (!frame_source_->GetNextFrame(&frame_packet)) {
        LOG_ERROR << "frame source get next frame fail";
        SendRtcpBye();
    } else {

        ++play_frames_;

        ++play_packets_;
        //  打包成RTP并发送

        if (update_ts) {
            last_rtp_ts_ += ts_duration_;
        }

        LOG_TRACE << "NALU " << frame_packet.type << ", length "
                  << frame_packet.size << ", ts " << last_rtp_ts_;

        AVPacketInfo info;
        info.payload_type = media_subsession_->payload_type();
        info.timestamp = last_rtp_ts_;
        info.ssrc = frame_source_->ssrc();

        rtp_sink_->Send(frame_packet, info);

        if (frame_packet.type == NALU_TYPE_PPS ||
            frame_packet.type == NALU_TYPE_SEI ||
            frame_packet.type == NALU_TYPE_SPS) {
            loop_->QueueInLoop(
                std::bind(&RtspStreamState::PlayOnce, this, false));
        } else if (frame_packet.size > 0) { // 计划下一次发送
            loop_->RunAfter(0.04,
                            std::bind(&RtspStreamState::PlayOnce, this, true));
        }
    }
}

} // namespace muduo_media