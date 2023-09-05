#include "logger/logger.h"
#include "media_session.h"
#include "rtsp_server.h"
#include <iostream>

int main() {

    muduo::log::Logger::set_log_level(muduo::log::Logger::DEBUG);

    muduo::net::InetAddress listen_addr(8554);

    muduo::event_loop::EventLoop loop;

    rtsp::RtspServer rtsp_server(&loop, listen_addr, "RtspServer", true);

    rtsp::MediaSessionPtr session(new rtsp::MediaSession("live"));

    rtsp_server.AddMediaSession(session);

    rtsp_server.Start();

    loop.Loop();

    return 0;
}