#include "file_media_subsession.h"

namespace muduo_media {

FileMediaSubsession::FileMediaSubsession(const std::string &filename)
    : filename_(filename) {}

FileMediaSubsession::~FileMediaSubsession() {}

} // namespace muduo_media