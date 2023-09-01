#include "logger/logger.h"
#include "rtsp_server.h"
#include <iostream>

int main() {

    muduo::log::Logger::set_log_level(muduo::log::Logger::DEBUG);

    muduo::net::InetAddress listen_addr(8554);

    muduo::event_loop::EventLoop loop;

    RtspServer rtsp_server(&loop, listen_addr, "RtspServer", true);

    rtsp_server.Start();

    loop.Loop();

    return 0;
}