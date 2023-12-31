#ifndef D6A17C46_BBE9_4050_AAAE_543E68DBB72D
#define D6A17C46_BBE9_4050_AAAE_543E68DBB72D

#include "media_source.h"

#include <memory>

namespace muduo_media {

class AVPacket;

class MultiFrameSource : public MediaSource {
public:
    MultiFrameSource();
    virtual ~MultiFrameSource() = default;

    virtual bool GetNextFrame(AVPacket *) = 0;

    uint32_t ssrc() const { return ssrc_; }
    void set_ssrc(uint32_t ssrc) { ssrc_ = ssrc; }

protected:
    uint32_t ssrc_;
};

using MultiFrameSourcePtr = std::shared_ptr<MultiFrameSource>;

} // namespace muduo_media

#endif /* D6A17C46_BBE9_4050_AAAE_543E68DBB72D */
