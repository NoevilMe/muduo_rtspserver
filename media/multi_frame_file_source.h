#ifndef __MULTI_FRAME_FILE_SOURCE_H__
#define __MULTI_FRAME_FILE_SOURCE_H__

#include "multi_frame_source.h"

#include <cstdio>

namespace muduo_media {

class MultiFrameFileSource : public MultiFrameSource {
public:
    MultiFrameFileSource(FILE *file);
    virtual ~MultiFrameFileSource();

protected:
    FILE *file_;
};

} // namespace muduo_media

#endif // __MULTI_FRAME_FILE_SOURCE_H__