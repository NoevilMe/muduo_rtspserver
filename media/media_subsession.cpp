#include "media_subsession.h"

#include <stdexcept>

namespace muduo_media {
MediaSubsession::MediaSubsession(unsigned int fps, unsigned int time_base)
    : track_id_(0), fps_(fps), time_base_(time_base) {}

MediaSubsession::~MediaSubsession() {}

std::string MediaSubsession::TrackId() {
    return std::string("track").append(std::to_string(track_id_));
}

unsigned int MediaSubsession::Duration() const {
    if (0 == fps_) {
        throw std::runtime_error("0 fps for duration calculation");
    }

    return time_base_ / fps_;
}

} // namespace muduo_media