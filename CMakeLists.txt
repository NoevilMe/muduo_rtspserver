cmake_minimum_required(VERSION 3.0)
project(muduo_media)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-g -O0")

set(SERVER_TOP ${CMAKE_SOURCE_DIR})

add_subdirectory(tinymuduo)

set(LIB_MEDIA_SRC
    media/av_packet.cpp
    media/rtcp.cpp
    media/media_subsession.cpp
    media/file_media_subsession.cpp
    media/h264_file_subsession.cpp
    media/rtp_sink.cpp
    media/multi_frame_rtp_sink.cpp
    media/multi_frame_source.cpp
    media/multi_frame_file_source.cpp
    media/byte_stream_file_source.cpp
    media/h264_file_source.cpp
    media/h264_video_rtp_sink.cpp)
add_library(media ${LIB_MEDIA_SRC})
target_include_directories(media PUBLIC ${SERVER_TOP} ${SERVER_TOP}/tinymuduo)

set(LIB_RTSP_SRC
    rtsp/rtsp_server.cpp
    rtsp/rtsp_connection.cpp
    rtsp/rtsp_message.cpp
    rtsp/utils.cpp
    rtsp/media_session.cpp
    rtsp/rtsp_session.cpp
    rtsp/stream_state.cpp
    rtsp/rtsp_stream_state.cpp)

add_library(rtsp ${LIB_RTSP_SRC})
target_link_libraries(rtsp PUBLIC media muduo_net)
target_include_directories(rtsp PUBLIC ${SERVER_TOP} ${SERVER_TOP}/tinymuduo)

set(MUDUO_MEDIA_SRC main.cpp)
add_executable(muduo_media_server ${MUDUO_MEDIA_SRC})
target_link_libraries(muduo_media_server PRIVATE rtsp)
