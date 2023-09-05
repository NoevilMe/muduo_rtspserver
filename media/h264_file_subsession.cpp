#include "h264_file_subsession.h"

namespace rtsp {
H264FileSubsession::H264FileSubsession() {}

H264FileSubsession::~H264FileSubsession() {}

std::string H264FileSubsession::GetSdp(uint track_id) {
    char media_sdp[200] = {0};
    snprintf(media_sdp, sizeof(media_sdp),
             "m=video 0 RTP/AVP 96\r\n"
             "a=rtpmap:96 H264/90000\r\n"
             "a=framerate:25\r\n"
             "a=control:track%u\r\n",
             track_id);
    return media_sdp;
}

} // namespace rtsp