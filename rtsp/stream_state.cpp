#include "stream_state.h"

namespace muduo_media {

StreamState::StreamState(muduo::event_loop::EventLoop *loop)
    : loop_(loop),
      playing_(false),
      play_times_(0),
      play_frames_(0),
      play_packets_(0) {}

StreamState::~StreamState() {}

} // namespace muduo_media