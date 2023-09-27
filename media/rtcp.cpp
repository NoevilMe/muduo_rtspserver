#include "rtcp.h"

#include "eventloop/endian.h"
#include "logger/logger.h"

#include <cstring>

namespace muduo_media {

std::string RtcpSRMessage::Serialize() {

    header.rc = 0;
    header.length = muduo::HostToNetwork16(
        (sizeof(header) + sizeof(sender_info)) / RTCP_LENGTH_DWORD - 1);
    header.pt = (uint8_t)RtcpPacketType::RTCP_SR;
    header.ssrc = muduo::HostToNetwork32(header.ssrc);

    sender_info.ts_msw = muduo::HostToNetwork32(sender_info.ts_msw);
    sender_info.ts_lsw = muduo::HostToNetwork32(sender_info.ts_lsw);
    sender_info.rtp_ts = muduo::HostToNetwork32(sender_info.rtp_ts);
    sender_info.packets = muduo::HostToNetwork32(sender_info.packets);
    sender_info.octets = muduo::HostToNetwork32(sender_info.octets);

    std::string binary;
    binary.resize(sizeof(header) + sizeof(sender_info));
    char *pdata = &binary[0];

    memcpy(pdata, &header, sizeof(header));
    memcpy(pdata + sizeof(header), &sender_info, sizeof(sender_info));

    LOG_DEBUG << "RtcpSRMessage " << binary.size();

    return binary;
}

bool RtcpSRMessage::Deserialize(const char *buf, size_t size) { return true; }

std::string RtcpRRMessage::Serialize() { return std::string(); }

bool RtcpRRMessage::Deserialize(const char *buf, size_t size) {
    if (!header.Valid()) {
        LOG_ERROR << "Invalid header";
    }

    for (uint8_t idx = 0; idx < header.rc; ++idx) {
        RtcpReportBlock block = {0};
        memcpy(&block, buf + idx * sizeof(RtcpReportBlock),
               sizeof(RtcpReportBlock));
        block.ssrc = muduo::NetworkToHost32(block.ssrc);
        block.lost_packets = muduo::NetworkToHost32(block.lost_packets);
        block.sequence = muduo::NetworkToHost32(block.sequence);
        block.jitter = muduo::NetworkToHost32(block.jitter);
        block.lsr = muduo::NetworkToHost32(block.lsr);
        block.dlsr = muduo::NetworkToHost32(block.dlsr);

        LOG_DEBUG << "report block " << report_blocks.size() << ", ssrc "
                  << block.ssrc << ", fraction lost " << block.fraction
                  << ", packets lost " << block.lost_packets
                  << ", highest sequence number " << block.sequence
                  << ", interarrival jitter " << block.jitter << ", last SR "
                  << block.lsr << ", delay since last SR " << block.dlsr;

        report_blocks.push_back(block);
    }

    return true;
}

std::string RtcpSDESMessage::Serialize() { return std::string(); }

bool RtcpSDESMessage::Deserialize(const char *buf, size_t size) {
    if (!header.Valid()) {
        LOG_ERROR << "Invalid header";
    }
    header.ssrc = 0; // no ssrc in header

    auto pdata = buf;

    while (pdata < buf + size) {
        RtcpSDESChunk chunk;
        memcpy(&chunk.ssrc, pdata, sizeof(chunk.ssrc));

        pdata += sizeof(chunk.ssrc);

        while (*pdata != (uint8_t)RtcpSDESItemType::END) {
            RtcpSDESItem item;
            memcpy(&item.head, pdata, 2); // 2字节
            pdata += 2;

            item.text.assign(pdata, item.head.length);

            LOG_DEBUG << "chunk " << chunks.size() << ", ssrc " << chunk.ssrc
                      << ", item " << chunk.items.size() << " type "
                      << item.head.type << ", length " << item.head.length
                      << ", text " << item.text;
            chunk.items.push_back(item);

            pdata += item.head.length;
        }

        chunks.push_back(chunk);

        while (*pdata == (uint8_t)RtcpSDESItemType::END) {
            ++pdata;
        }
    }

    return true;
}

std::string RtcpBYEMessage::Serialize() {

    header.rc = 1;
    header.length =
        muduo::HostToNetwork16(sizeof(header) / RTCP_LENGTH_DWORD - 1);
    header.pt = (uint8_t)RtcpPacketType::RTCP_BYE;
    header.ssrc = muduo::HostToNetwork32(header.ssrc);

    std::string binary;
    binary.resize(sizeof(header));
    char *pdata = &binary[0];

    memcpy(pdata, &header, sizeof(header));

    LOG_DEBUG << "RtcpBYEMessage " << binary.size();

    return binary;
}

bool RtcpBYEMessage::Deserialize(const char *buf, size_t size) {
    LOG_DEBUG << "RtcpBYEMessage " << header.ssrc;
    return true;
}

std::string RtcpAPPMessage::Serialize() { return std::string(); }

bool RtcpAPPMessage::Deserialize(const char *buf, size_t size) { return true; }

} // namespace muduo_media