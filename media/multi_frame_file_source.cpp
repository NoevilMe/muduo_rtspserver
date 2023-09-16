#include "multi_frame_file_source.h"

namespace muduo_media {

MultiFrameFileSource::MultiFrameFileSource(FILE *file) : file_(file) {}

MultiFrameFileSource::~MultiFrameFileSource() {}

} // namespace muduo_media