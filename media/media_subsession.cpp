#include "media_subsession.h"

namespace muduo_media {
MediaSubsession::MediaSubsession(unsigned int fps, unsigned int time_base)
    : track_id_(0), fps_(fps), time_base_(time_base) {}

MediaSubsession::~MediaSubsession() {}

std::string MediaSubsession::TrackId() {
    return std::string("track").append(std::to_string(track_id_));
}

} // namespace muduo_media