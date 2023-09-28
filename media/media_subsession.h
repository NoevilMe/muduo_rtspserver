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
    MediaSubsession(unsigned int fps = 25, unsigned int time_base = 90000);
    virtual ~MediaSubsession();

    std::string TrackId();

    unsigned int track_id() const { return track_id_; }
    void set_track_id(unsigned int id) { track_id_ = id; }

    unsigned int fps() const { return fps_; }
    void set_fps(unsigned int fps) { fps_ = fps; }

    unsigned int time_base() const { return time_base_; }
    void set_time_base(unsigned int time_base) { time_base_ = time_base; }

    // 时间戳步进
    unsigned int Duration() const;

    unsigned char payload_type() const { return payload_type_; }
    void set_payload_type(unsigned char type) { payload_type_ = type; }

    virtual std::string GetSdp() = 0;

    virtual RtpSinkPtr
    NewRtpSink(const std::shared_ptr<muduo::net::TcpConnection> &tcp_conn,
               int8_t rtp_channel) = 0;

    virtual RtpSinkPtr NewRtpSink(
        const std::shared_ptr<muduo::net::UdpVirtualConnection> &udp_conn) = 0;

    virtual MultiFrameSourcePtr NewMultiFrameSouce() = 0;

protected:
    unsigned int track_id_;
    unsigned int fps_;
    unsigned int time_base_;
    unsigned char payload_type_;
};

using MediaSubsessionPtr = std::shared_ptr<MediaSubsession>;

} // namespace muduo_media

#endif /* F31514DC_F2FD_4627_8CA5_A941C5ED1394 */
