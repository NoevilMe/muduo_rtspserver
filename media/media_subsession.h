#ifndef F31514DC_F2FD_4627_8CA5_A941C5ED1394
#define F31514DC_F2FD_4627_8CA5_A941C5ED1394

#include "multi_frame_source.h"
#include "rtp_sink.h"

#include <string>

namespace muduo {
namespace net {
class UdpVirtualConnection;
class TcpConnection;
} // namespace net
} // namespace muduo

namespace muduo_media {

class MediaSubsession {
public:
    MediaSubsession();
    virtual ~MediaSubsession();

    std::string TrackId();

    unsigned int track_id() const { return track_id_; }
    void set_track_id(unsigned int id) { track_id_ = id; }

    virtual std::string GetSdp() = 0;

    virtual RtpSinkPtr
    NewRtpSink(const std::shared_ptr<muduo::net::TcpConnection> &tcp_conn,
               int8_t rtp_channel) = 0;

    virtual RtpSinkPtr NewRtpSink(
        const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn) = 0;

    virtual MultiFrameSourcePtr NewMultiFrameSouce() = 0;

protected:
    unsigned int track_id_;
};

using MediaSubsessionPtr = std::shared_ptr<MediaSubsession>;

} // namespace muduo_media

#endif /* F31514DC_F2FD_4627_8CA5_A941C5ED1394 */
