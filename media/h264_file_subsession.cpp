#include "h264_file_subsession.h"
#include "h264_file_source.h"
#include "h264_video_rtp_sink.h"

#include <cstring>

namespace muduo_media {
H264FileSubsession::H264FileSubsession(const std::string &filename)
    : FileMediaSubsession(filename) {}

H264FileSubsession::~H264FileSubsession() {}

std::string H264FileSubsession::GetSdp() {
    char media_sdp[200] = {0};
    snprintf(media_sdp, sizeof(media_sdp),
             "m=video 0 RTP/AVP 96\r\n"
             "a=rtpmap:96 H264/90000\r\n"
             "a=framerate:25\r\n"
             "a=control:%s\r\n",
             TrackId().data());
    return media_sdp;
}

// bool H264FileSubsession::IsOpened() const { return m_file != nullptr; }

// int H264FileSubsession::ReadFrame(char *in_buf, int in_buf_size, bool *end) {
//     if (m_file == NULL) {
//         return -1;
//     }

//     int bytes_read = (int)fread(m_buf, 1, m_buf_size, m_file);
//     if (bytes_read == 0) {
//         fseek(m_file, 0, SEEK_SET);
//         m_count = 0;
//         m_bytes_used = 0;
//         bytes_read = (int)fread(m_buf, 1, m_buf_size, m_file);
//         if (bytes_read == 0) {
//             this->Close();
//             return -1;
//         }
//     }

//     bool is_find_start = false, is_find_end = false;
//     int i = 0, start_code = 3;
//     *end = false;

//     for (i = 0; i < bytes_read - 5; i++) {
//         if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1) {
//             start_code = 3;
//         } else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 &&
//                    m_buf[i + 3] == 1) {
//             start_code = 4;
//         } else {
//             continue;
//         }

//         if (((m_buf[i + start_code] & 0x1F) == 0x5 ||
//              (m_buf[i + start_code] & 0x1F) == 0x1) &&
//             ((m_buf[i + start_code + 1] & 0x80) == 0x80)) {
//             is_find_start = true;
//             i += 4;
//             break;
//         }
//     }

//     for (; i < bytes_read - 5; i++) {
//         if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1) {
//             start_code = 3;
//         } else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 &&
//                    m_buf[i + 3] == 1) {
//             start_code = 4;
//         } else {
//             continue;
//         }

//         if (((m_buf[i + start_code] & 0x1F) == 0x7) ||
//             ((m_buf[i + start_code] & 0x1F) == 0x8) ||
//             ((m_buf[i + start_code] & 0x1F) == 0x6) ||
//             (((m_buf[i + start_code] & 0x1F) == 0x5 ||
//               (m_buf[i + start_code] & 0x1F) == 0x1) &&
//              ((m_buf[i + start_code + 1] & 0x80) == 0x80))) {
//             is_find_end = true;
//             break;
//         }
//     }

//     bool flag = false;
//     if (is_find_start && !is_find_end && m_count > 0) {
//         flag = is_find_end = true;
//         i = bytes_read;
//         *end = true;
//     }

//     if (!is_find_start || !is_find_end) {
//         this->Close();
//         return -1;
//     }

//     int size = (i <= in_buf_size ? i : in_buf_size);
//     memcpy(in_buf, m_buf, size);

//     if (!flag) {
//         m_count += 1;
//         m_bytes_used += i;
//     } else {
//         m_count = 0;
//         m_bytes_used = 0;
//     }

//     fseek(m_file, m_bytes_used, SEEK_SET);
//     return size;
// }

RtpSinkPtr H264FileSubsession::NewRtpSink(
    const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn) {
    return std::make_shared<H264VideoRtpSink>(udp_conn);
}

MultiFrameSourcePtr H264FileSubsession::NewMultiFrameSouce() {

    FILE *file = fopen(filename_.data(), "rb");

    std::shared_ptr<H264FileSource> filesource(new H264FileSource(file));

    return filesource;
}

} // namespace muduo_media