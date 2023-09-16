#ifndef F3C5294E_6DAC_479D_A5B0_23A7A59A906A
#define F3C5294E_6DAC_479D_A5B0_23A7A59A906A

#include "media_subsession.h"

namespace muduo_media {

class FileMediaSubsession : public MediaSubsession {
protected: // we're a virtual base class
    FileMediaSubsession(const std::string &filename);
    virtual ~FileMediaSubsession();

protected:
    std::string filename_;
    u_int64_t filesize_;
};

} // namespace muduo_media

#endif /* F3C5294E_6DAC_479D_A5B0_23A7A59A906A */
