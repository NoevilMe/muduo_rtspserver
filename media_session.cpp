#include "media_session.h"
#include "eventloop/event_loop.h"
#include "media/av_packet.h"

#include <cstring>
#include <ctime>

namespace muduo_media {

MediaSession::MediaSession(const std::string &path) : name_(path) {}

MediaSession::~MediaSession() {}

void MediaSession::AddSubsession(const MediaSubsessionPtr &subsession) {
    std::size_t index = subsessions_.size();
    subsession->set_track_id(index);
    subsessions_.insert(std::make_pair(subsession->TrackId(), subsession));
}

MediaSubsessionPtr MediaSession::GetSubsession(const std::string &track) {
    auto it = subsessions_.find(track);
    if (it != subsessions_.end()) {
        return it->second;
    }

    return nullptr;
}

bool MediaSession::SubsessionExists(const std::string &track) {
    return subsessions_.find(track) != subsessions_.end();
}

std::string MediaSession::GetMethodsAsString() {
    std::string methods("OPTIONS, DESCRIBE, SETUP, TRARDOWN, PLAY");
    return methods;
}

std::string MediaSession::BuildSdp() {

    // TODO:
    std::string ip = "0.0.0.0";

    char session_sdp[100] = {0};
    snprintf(session_sdp, sizeof(session_sdp),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n",
             (long)std::time(NULL), ip.data());

    std::string data(session_sdp);

    for (auto &&i : subsessions_) {
        std::string media_sdp = i.second->GetSdp();
        data.append(media_sdp);
    }

    return data;
}

void MediaSession::Play(muduo::event_loop::EventLoop *loop,
                        unsigned int rtsp_session_id,
                        std::vector<std::string> track) {}

void MediaSession::PlayReadFrame(std::shared_ptr<MediaSubsession> subsession) {
    // int buf_size = 2000000;
    // std::unique_ptr<uint8_t> frame_buf(new uint8_t[buf_size]);

    // bool end_of_frame = false;
    // int frame_size =
    //     subsession->ReadFrame((char *)frame_buf.get(), buf_size,
    //     &end_of_frame);
    // if (frame_size > 0) {
    //     AVPacket videoFrame = {0};
    //     videoFrame.type = 0;
    //     videoFrame.size = frame_size;
    //     videoFrame.timestamp = 0;
    //     videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
    //     memcpy(videoFrame.buffer.get(), frame_buf.get(), videoFrame.size);
    // }
}

} // namespace muduo_media