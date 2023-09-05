#include "logger/logger.h"
#include "media/h264_file_subsession.h"
#include "media_session.h"
#include "rtsp_server.h"
#include <iostream>

int main() {

    muduo::log::Logger::set_log_level(muduo::log::Logger::DEBUG);

    muduo::net::InetAddress listen_addr(8554);

    muduo::event_loop::EventLoop loop;

    rtsp::RtspServer rtsp_server(&loop, listen_addr, "RtspServer", true);

    rtsp::MediaSessionPtr session(new rtsp::MediaSession("live"));

    std::shared_ptr<rtsp::H264FileSubsession> h264_file(
        new rtsp::H264FileSubsession);
    std::shared_ptr<rtsp::MediaSubsession> h264_subsession =
        std::static_pointer_cast<rtsp::MediaSubsession>(h264_file);
    session->AddSubsession(h264_subsession);

    rtsp_server.AddMediaSession(session);

    rtsp_server.Start();

    loop.Loop();

    return 0;
}