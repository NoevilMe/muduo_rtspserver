#ifndef ED6E2ABA_9853_4C81_AA39_05DE3C492A49
#define ED6E2ABA_9853_4C81_AA39_05DE3C492A49

#include "av_packet.h"
#include "media_sink.h"

#include <memory>

namespace muduo_media {

class RtpSink : public MediaSink {
public:
    RtpSink() : packets_(0), octets_(0) {}
    virtual ~RtpSink() = default;

    virtual void Send(const unsigned char *data, int len,
                      const std::shared_ptr<void> &info) = 0;

    virtual void Send(const AVPacket &pkt, const AVPacketInfo &info) = 0;

    uint32_t packets() const { return packets_; }
    uint32_t octets() const { return octets_; }

protected:
    uint32_t packets_;
    uint32_t octets_;
};

using RtpSinkPtr = std::shared_ptr<RtpSink>;

} // namespace muduo_media

#endif /* ED6E2ABA_9853_4C81_AA39_05DE3C492A49 */
