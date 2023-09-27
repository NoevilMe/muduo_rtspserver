#include "file_media_subsession.h"

namespace muduo_media {
FileMediaSubsession::FileMediaSubsession(const std::string &filename,
                                         unsigned int fps,
                                         unsigned int time_base)
    : MediaSubsession(fps, time_base), filename_(filename) {}

FileMediaSubsession::~FileMediaSubsession() {}

} // namespace muduo_media