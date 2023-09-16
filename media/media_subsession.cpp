#include "media_subsession.h"

namespace muduo_media {
MediaSubsession::MediaSubsession() {}

MediaSubsession::~MediaSubsession() {}

std::string MediaSubsession::TrackId() {
    return std::string("track").append(std::to_string(track_id_));
}

} // namespace muduo_media