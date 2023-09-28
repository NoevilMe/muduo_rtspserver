#ifndef BFF1D915_36C9_46D9_8CAE_4C2B2782D200
#define BFF1D915_36C9_46D9_8CAE_4C2B2782D200

#include "av_packet.h"
#include "multi_frame_file_source.h"

namespace muduo_media {

class H264FileSource : public MultiFrameFileSource {
public:
    H264FileSource(FILE *file);
    ~H264FileSource();

    bool GetNextFrame(AVPacket *) override;

private:
    int GetNextNALU(H264Nalu *nalu);
};

} // namespace muduo_media

#endif /* BFF1D915_36C9_46D9_8CAE_4C2B2782D200 */
