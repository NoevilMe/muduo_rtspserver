#ifndef BFF1D915_36C9_46D9_8CAE_4C2B2782D200
#define BFF1D915_36C9_46D9_8CAE_4C2B2782D200

#include "multi_frame_file_source.h"

#include <cstdio>

namespace muduo_media {

typedef struct {
    int statcode_length;   //! 4 for parameter sets and first slice in
                           //! picture, 3 for everything else (suggested)
    int forbidden_bit;     //! should be always FALSE
    int nal_reference_bit; //! NALU_PRIORITY_xxxx
    int nal_unit_type;     //! NALU_TYPE_xxxx

    size_t len;      //! Length of the NAL unit (Excluding the start code, which
                     //! does not belong to the NALU)
    size_t max_size; //! Nal Unit Buffer size
    unsigned char *buf; //! contains the first byte followed by the EBSP

    void AppendData(unsigned char *data_buf, size_t data_len);
} H264Nalu;

class H264FileSource : public MultiFrameFileSource {
public:
    H264FileSource(FILE *file);
    ~H264FileSource();

    bool GetNextFrame(AVPacket *) override;

private:
    int GetAnnexbNALU(H264Nalu *nalu);

private:
    char *m_buf = nullptr;
    int m_buf_size = 0;
    int m_bytes_used = 0;
    int m_count = 0;
};

} // namespace muduo_media

#endif /* BFF1D915_36C9_46D9_8CAE_4C2B2782D200 */
