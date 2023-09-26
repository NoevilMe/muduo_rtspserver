#ifndef D6A17C46_BBE9_4050_AAAE_543E68DBB72D
#define D6A17C46_BBE9_4050_AAAE_543E68DBB72D

#include "media_source.h"

#include <memory>

namespace muduo_media {

class AVPacket;

class MultiFrameSource : public MediaSource {
public:
    MultiFrameSource() = default;
    virtual ~MultiFrameSource() = default;

    virtual bool GetNextFrame(AVPacket *) = 0;
    virtual uint32_t SSRC() = 0;
};

using MultiFrameSourcePtr = std::shared_ptr<MultiFrameSource>;

} // namespace muduo_media

#endif /* D6A17C46_BBE9_4050_AAAE_543E68DBB72D */
