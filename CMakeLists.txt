cmake_minimum_required(VERSION 3.0)
project(muduo_rtspserver)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-g")

set(SERVER_TOP ${CMAKE_SOURCE_DIR})

add_subdirectory(tinymuduo)

add_executable(
  rtsp_server
  main.cpp
  rtsp_server.cpp
  rtsp_connection.cpp
  rtsp_message.cpp
  utils.cpp
  media_session.cpp
  media/media_subsession.cpp
  media/h264_file_subsession.cpp)
target_link_libraries(rtsp_server PRIVATE muduo_net pthread)
target_include_directories(rtsp_server PUBLIC ${SERVER_TOP}/tinymuduo)
