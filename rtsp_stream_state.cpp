#include "rtsp_stream_state.h"
#include "eventloop/endian.h"
#include "logger/logger.h"
#include "media/av_packet.h"
#include "media/rtcp.h"
#include "media/rtp.h"

namespace muduo_media {

RtspStreamState::RtspStreamState(muduo::event_loop::EventLoop *loop,
                                 const MediaSubsessionPtr &media_subsession,
                                 const RtpSinkPtr &rtp_sink,
                                 const MultiFrameSourcePtr &frame_source)
    : StreamState(loop),
      media_subsession_(media_subsession),
      rtp_sink_(rtp_sink),
      frame_source_(frame_source) {

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
    loop_->QueueInLoop(std::bind(&RtspStreamState::PlayOnce, this));
}

void RtspStreamState::Teardown() { playing_ = false; }

void RtspStreamState::ParseRTP(const char *buf, size_t size) {}

void RtspStreamState::ParseRTCP(const char *buf, size_t size) {
    size_t parse_size = 0;
    size_t left_size = size;
    while (left_size > sizeof(RtcpHeader)) {
        RtcpHeader rtcp = {0};
        memcpy(&rtcp, buf + parse_size, sizeof(RtcpHeader));
        rtcp.length = muduo::NetworkToHost16(rtcp.length);
        rtcp.ssrc = muduo::NetworkToHost32(rtcp.ssrc);

        size_t packet_size = (rtcp.length + 1) * RTCP_LENGTH_WORDS;

        LOG_DEBUG << "RTCP V " << rtcp.v << ", P " << rtcp.p << ", RC "
                  << rtcp.rc << ", PT " << rtcp.pt << ", length " << rtcp.length
                  << ", ssrc " << rtcp.ssrc << ", RTCP packet size "
                  << packet_size;

        if (rtcp.pt == (uint8_t)RtcpPacketType::RTCP_RR) {
            OnRtcpRR(rtcp.rc, buf + parse_size + sizeof(RtcpHeader),
                     (rtcp.length - 1) * RTCP_LENGTH_WORDS);
        } else if (rtcp.pt == (uint8_t)RtcpPacketType::RTCP_SDES) {
            OnRtcpSDES(rtcp.rc,
                       buf + parse_size + sizeof(RtcpHeader) -
                           sizeof(rtcp.ssrc), // ssrc belongs to chunk
                       rtcp.length * RTCP_LENGTH_WORDS);
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

void RtspStreamState::OnRtcpRR(uint8_t rc, const char *buf, size_t /*size*/) {
    for (uint8_t idx = 0; idx < rc; ++idx) {
        RtcpReportBlock block = {0};
        memcpy(&block, buf + idx * sizeof(RtcpReportBlock),
               sizeof(RtcpReportBlock));
        block.ssrc = muduo::NetworkToHost32(block.ssrc);
        block.lost_packets = muduo::NetworkToHost32(block.lost_packets);
        block.sequence = muduo::NetworkToHost32(block.sequence);
        block.jitter = muduo::NetworkToHost32(block.jitter);
        block.lsr = muduo::NetworkToHost32(block.lsr);
        block.dlsr = muduo::NetworkToHost32(block.dlsr);
        LOG_DEBUG << "report block ssrc " << block.ssrc << ", fraction lost "
                  << block.fraction << ", packets lost " << block.lost_packets
                  << ", highest sequence number " << block.sequence
                  << ", interarrival jitter " << block.jitter << ", last SR "
                  << block.lsr << ", delay since last SR " << block.dlsr;
    }
}

void RtspStreamState::OnRtcpSDES(uint8_t rc, const char *buf, size_t size) {
    auto pdata = buf;

    RtcpSDESChunk chunk{0};
    memcpy(&chunk, pdata, sizeof(RtcpSDESChunk));
    chunk.ssrc = muduo::NetworkToHost32(chunk.ssrc);

    pdata += sizeof(RtcpSDESChunk);

    LOG_DEBUG << "chunk item type " << chunk.items[0].type << ", length "
              << chunk.items[0].length << ", text "
              << muduo::StringPiece(pdata, chunk.items[0].length);

    pdata += chunk.items[0].length;
    LOG_DEBUG << "left SDES " << buf + size - pdata;
}

void RtspStreamState::PlayOnce() {
    if (!playing_) {
        LOG_DEBUG << "not playing at " << this;
        return;
    }

    // IMPORTANT!!! In timer, this object might be released.
    if (!frame_source_) {
        LOG_WARN << "PlayOnce cancel at " << this;
        return;
    }

    AVPacket frame_packet;
    // 读取一帧, 获取NALU
    // 同一帧可能有多个NALU，如果是同一个帧的多个NALU则需要立即发送
    if (!frame_source_->GetNextFrame(&frame_packet)) {
        LOG_ERROR << "frame source get next frame fail";
        if (goodbye_cb_) {
            goodbye_cb_(frame_source_->SSRC());
        }
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
        rtp_header->seq = 0; // rtp sink will modify this value
        rtp_header->timestamp = muduo::HostToNetwork32(
            frame_packet.timestamp); // 需要根据源计算 htonl()
        rtp_header->ssrc =
            muduo::HostToNetwork32(frame_source_->SSRC()); // 信号源id

        rtp_sink_->Send(frame_packet.buffer.get(), frame_packet.size,
                        rtp_header);

        // 计划下一次发送
        if (frame_packet.size > 0) {
            loop_->RunAfter(0.04, std::bind(&RtspStreamState::PlayOnce, this));
        }
    }
}

} // namespace muduo_media