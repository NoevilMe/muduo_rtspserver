#ifndef D60BAB5F_BC89_47FC_B900_0C31938E7510
#define D60BAB5F_BC89_47FC_B900_0C31938E7510

#include <memory>
#include <string>

namespace rtsp {

class MediaSession {
public:
    MediaSession(const std::string &path);
    ~MediaSession();

    const std::string &path() const { return path_; }

    std::string BuildSdp();

private:
    std::string path_;
};

using MediaSessionPtr = std::shared_ptr<MediaSession>;

} // namespace rtsp

#endif /* D60BAB5F_BC89_47FC_B900_0C31938E7510 */
