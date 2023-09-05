#ifndef D60BAB5F_BC89_47FC_B900_0C31938E7510
#define D60BAB5F_BC89_47FC_B900_0C31938E7510

#include "media/media_subsession.h"

#include <memory>
#include <string>
#include <vector>

namespace rtsp {

/// @brief rtsp url中path部分表示的一种资源服务
class MediaSession {
public:
    MediaSession(const std::string &path);
    ~MediaSession();

    const std::string &path() const { return path_; }

    void AddSubsession(const std::shared_ptr<MediaSubsession> &subsession);

    std::string BuildSdp();

private:
    std::string path_;
    std::vector<std::shared_ptr<MediaSubsession>> subsessions_;
};

using MediaSessionPtr = std::shared_ptr<MediaSession>;

} // namespace rtsp

#endif /* D60BAB5F_BC89_47FC_B900_0C31938E7510 */
