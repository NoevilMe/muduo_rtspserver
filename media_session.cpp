#include "media_session.h"

#include <ctime>

namespace rtsp {

MediaSession::MediaSession(const std::string &path) : path_(path) {}

MediaSession::~MediaSession() {}

void MediaSession::AddSubsession(
    const std::shared_ptr<MediaSubsession> &subsession) {
    subsessions_.push_back(subsession);
}

std::string MediaSession::BuildSdp() {

    // TODO:
    std::string ip = "0.0.0.0";

    char session_sdp[100] = {0};
    snprintf(session_sdp, sizeof(session_sdp),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n",
             (long)std::time(NULL), ip.data());

    std::string data(session_sdp);

    for (size_t i = 0; i < subsessions_.size(); ++i) {
        auto &subsession = subsessions_[i];
        std::string media_sdp = subsession->GetSdp(i);
        data.append(media_sdp);
    }

    return data;
}

} // namespace rtsp