#ifndef D6691F0E_BEE2_4A23_B41A_E551C272E83F
#define D6691F0E_BEE2_4A23_B41A_E551C272E83F

#include "multi_frame_file_source.h"

namespace muduo_media {

class ByteStreamFileSource : public MultiFrameFileSource {
public:
    ByteStreamFileSource() = default;
    virtual ~ByteStreamFileSource() = default;
};

} // namespace muduo_media

#endif /* D6691F0E_BEE2_4A23_B41A_E551C272E83F */
