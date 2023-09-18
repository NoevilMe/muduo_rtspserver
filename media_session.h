#ifndef D60BAB5F_BC89_47FC_B900_0C31938E7510
#define D60BAB5F_BC89_47FC_B900_0C31938E7510

#include "eventloop/event_loop.h"
#include "media/media_subsession.h"

#include <map>
#include <memory>
#include <string>

namespace muduo_media {

/// @brief muduo_media url中path部分表示的一种资源服务
class MediaSession {
public:
    MediaSession(const std::string &path);
    ~MediaSession();

    const std::string &name() const { return name_; }

    void AddSubsession(const MediaSubsessionPtr &subsession);
    MediaSubsessionPtr GetSubsession(const std::string &track);

    bool SubsessionExists(const std::string &track);

    std::string GetMethodsAsString();

    std::string BuildSdp();

    void Play(muduo::event_loop::EventLoop *loop, unsigned int rtsp_session_id,
              std::vector<std::string> track);

private:
    void PlayReadFrame(std::shared_ptr<MediaSubsession> subsession);

private:
    std::string name_;
    std::map<std::string, std::shared_ptr<MediaSubsession>> subsessions_;
};

using MediaSessionPtr = std::shared_ptr<MediaSession>;

} // namespace muduo_media

#endif /* D60BAB5F_BC89_47FC_B900_0C31938E7510 */
