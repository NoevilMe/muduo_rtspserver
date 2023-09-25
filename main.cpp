#include "logger/logger.h"
#include "media/h264_file_subsession.h"
#include "media_session.h"
#include "rtsp_server.h"
#include <iostream>

int main() {

    muduo::log::Logger::set_log_level(muduo::log::Logger::TRACE);

    muduo::net::InetAddress listen_addr(8554);

    muduo::event_loop::EventLoop loop;

    muduo_media::RtspServer rtsp_server(&loop, listen_addr, "RtspServer", true);

    muduo_media::MediaSessionPtr session(new muduo_media::MediaSession("live"));

    std::shared_ptr<muduo_media::H264FileSubsession> h264_file(
        new muduo_media::H264FileSubsession("test2.h264"));
    std::shared_ptr<muduo_media::MediaSubsession> h264_subsession =
        std::static_pointer_cast<muduo_media::MediaSubsession>(h264_file);
    session->AddSubsession(h264_subsession);

    rtsp_server.AddMediaSession(session);

    rtsp_server.Start();

    loop.Loop();

    return 0;
}