#ifndef A829ACA4_FC85_4098_AA0A_92C3C824D67E
#define A829ACA4_FC85_4098_AA0A_92C3C824D67E

#include "media_subsession.h"

namespace rtsp {

class H264FileSubsession : public MediaSubsession {

public:
    H264FileSubsession();
    ~H264FileSubsession();

    std::string GetSdp(uint track_id) override;
};

} // namespace rtsp

#endif /* A829ACA4_FC85_4098_AA0A_92C3C824D67E */
