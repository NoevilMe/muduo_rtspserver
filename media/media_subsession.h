#ifndef F31514DC_F2FD_4627_8CA5_A941C5ED1394
#define F31514DC_F2FD_4627_8CA5_A941C5ED1394

#include <string>

namespace rtsp {

class MediaSubsession {
public:
    MediaSubsession();
    virtual ~MediaSubsession();

    virtual std::string GetSdp(uint track_id) = 0;

    static std::string TrackId(uint track_id);
};

} // namespace rtsp

#endif /* F31514DC_F2FD_4627_8CA5_A941C5ED1394 */
