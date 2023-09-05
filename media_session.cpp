#include "media_session.h"

#include <ctime>

namespace rtsp {

MediaSession::MediaSession(const std::string &path) : path_(path) {}

MediaSession::~MediaSession() {}

std::string MediaSession::BuildSdp() {

    // TODO:
    std::string ip = "10.10.10.80";

    char session_sdp[100] = {0};
    snprintf(session_sdp, sizeof(session_sdp),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n",
             (long)std::time(NULL), ip.data());

    char media_sdp[200] = {0};
    snprintf(media_sdp, sizeof(media_sdp),
             "m=video 0 RTP/AVP 96\r\n"
             "a=rtpmap:96 H264/90000\r\n"
             "a=framerate:25\r\n"
             "a=control:track0\r\n");

    std::string data(session_sdp);
    data.append(media_sdp);
    return data;
}

} // namespace rtsp