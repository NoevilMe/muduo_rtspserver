#ifndef D8B36F66_7BE7_4D69_9FBA_3B8A2347C66B
#define D8B36F66_7BE7_4D69_9FBA_3B8A2347C66B

#include "eventloop/event_loop.h"

namespace muduo_media {

/// @brief RtspSession中表示当前流的状态
class StreamState {
public:
    StreamState(muduo::event_loop::EventLoop *loop);
    virtual ~StreamState();

    bool playing() const { return playing_; }
    size_t play_times() const { return play_times_; }
    size_t play_frames() const { return play_frames_; }
    size_t play_packets() const { return play_packets_; }

    virtual void Play() = 0;
    virtual void Teardown() = 0;

    virtual void ParseRTP(const char *buf, size_t size) = 0;
    virtual void ParseRTCP(const char *buf, size_t size) = 0;

protected:
    muduo::event_loop::EventLoop *loop_;
    bool playing_;
    size_t play_times_;
    size_t play_frames_;
    size_t play_packets_;
};

using StreamStatePtr = std::shared_ptr<StreamState>;

} // namespace muduo_media

#endif /* D8B36F66_7BE7_4D69_9FBA_3B8A2347C66B */
