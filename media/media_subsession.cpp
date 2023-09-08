#include "media_subsession.h"

namespace rtsp {
MediaSubsession::MediaSubsession() {}

MediaSubsession::~MediaSubsession() {}

std::string MediaSubsession::TrackId(uint track_id) {
    return std::string("track").append(std::to_string(track_id));
}

} // namespace rtsp