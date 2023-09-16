#ifndef FF6A389F_70C9_42BD_979F_FFA28F1B7C2E
#define FF6A389F_70C9_42BD_979F_FFA28F1B7C2E

#include "rtp_sink.h"

namespace muduo_media {

class MultiFrameRtpSink : public RtpSink {
public:
    MultiFrameRtpSink() = default;
    virtual ~MultiFrameRtpSink() = default;
};

} // namespace muduo_media

#endif /* FF6A389F_70C9_42BD_979F_FFA28F1B7C2E */
