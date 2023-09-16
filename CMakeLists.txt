cmake_minimum_required(VERSION 3.0)
project(muduo_rtspserver)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-g")

set(SERVER_TOP ${CMAKE_SOURCE_DIR})

add_subdirectory(tinymuduo)

set(MEDIA_SRC
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

add_library(media ${MEDIA_SRC})
target_include_directories(media PUBLIC ${SERVER_TOP} ${SERVER_TOP}/tinymuduo)

add_executable(
  rtsp_server
  main.cpp
  rtsp_server.cpp
  rtsp_connection.cpp
  rtsp_message.cpp
  utils.cpp
  media_session.cpp
  rtsp_session.cpp
  rtp_connection.cpp
  stream_state.cpp
  rtsp_stream_state.cpp)
target_link_libraries(rtsp_server PRIVATE muduo_net pthread media)
target_include_directories(rtsp_server PUBLIC ${SERVER_TOP}/tinymuduo)
