#include "h264_file_source.h"
#include "defs.h"
#include "eventloop/timestamp.h"
#include "logger/logger.h"
#include "media/av_packet.h"

#include <cstring>
#include <random>

namespace muduo_media {



static bool MatchStartCode3Bytes(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1)
        return false; // 0x000001?
    else
        return true;
}

static bool MatchStartCode4Bytes(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1)
        return false; // 0x00000001?
    else
        return true;
}

H264FileSource::H264FileSource(FILE *file) : MultiFrameFileSource(file) {
    LOG_DEBUG << "H264FileSource::ctor at " << this;

    std::random_device rd;
    ssrc_ = rd() & 0xFFFFFFFF;
}

H264FileSource::~H264FileSource() {
    LOG_DEBUG << "H264FileSource::dtor at " << this;
}

int H264FileSource::GetNextNALU(H264Nalu *nalu) {
    nalu->len = 0; // reset immediately
    nalu->statcode_length = 0;
    // >= kInterleavedFrameSize + RTP_HEADER_SIZE
    nalu->prepend_size = defs::kBufPrependSize;

    size_t data_buf_len = 10000;
    unsigned char *data_buf = new unsigned char[data_buf_len];

    size_t read_len = fread(data_buf, 1, data_buf_len, file_);
    if (read_len < 3) {
        delete[] data_buf;
        return 0;
    }

    // data cursor
    unsigned char *pdata = data_buf;
    // pdata must be smaller than this
    unsigned char *pdata_end = data_buf + read_len;

    if (!MatchStartCode3Bytes(pdata)) {
        if (read_len < 4) {
            delete[] data_buf;
            return 0;
        }
        if (!MatchStartCode4Bytes(pdata)) {
            delete[] data_buf;
            return -1;
        } else {
            nalu->statcode_length = 4;
            pdata += 4;
        }
    } else {
        nalu->statcode_length = 3;
        pdata += 3;
    }

    int next_startcode_length = 0;

    ssize_t rewind = 0;

    while (pdata != pdata_end) {
        if (!MatchStartCode4Bytes(pdata)) {
            if (MatchStartCode3Bytes(pdata)) {
                next_startcode_length = 3;
                break;
            }
        } else {
            next_startcode_length = 4;
            break;
        }

        ++pdata;

        if (pdata == pdata_end) {
            if (feof(file_)) {
                break;
            }

            rewind = -3;
            if (0 != fseek(file_, rewind, SEEK_CUR)) {
                delete[] data_buf;
                printf("GetNextNALU: Cannot fseek in the bit stream file");
                return -1;
            }

            pdata -= 3;

            ssize_t cur_data_len = pdata - data_buf;
            if (nalu->len == 0) { // first buf
                nalu->AppendData(data_buf + nalu->statcode_length,
                                 cur_data_len - nalu->statcode_length);
            } else {
                nalu->AppendData(data_buf, cur_data_len);
            }

            read_len = fread(data_buf, 1, data_buf_len, file_);
            if (read_len <= 0) {
                delete[] data_buf;
                return -1;
            }
            pdata_end = data_buf + read_len; // update
            pdata = data_buf;
        }
    }

    // Here, we have found another start code (and read length of startcode
    // bytes more than we should have.  Hence, go back in the file
    rewind = pdata - pdata_end;

    if (0 != fseek(file_, rewind, SEEK_CUR)) {
        delete[] data_buf;
        printf("GetNextNALU: Cannot fseek in the bit stream file");
        return -1;
    }

    ssize_t cur_data_len = pdata - data_buf;
    if (nalu->len) {
        nalu->AppendData(data_buf, cur_data_len);
    } else {
        nalu->AppendData(data_buf + nalu->statcode_length,
                         cur_data_len - nalu->statcode_length);
    }
    delete[] data_buf;

    nalu->forbidden_bit = nalu->buf[nalu->prepend_size] & 0x80;     // 1 bit
    nalu->nal_reference_idc = nalu->buf[nalu->prepend_size] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[nalu->prepend_size]) & 0x1f;   // 5 bit

    return nalu->len + nalu->statcode_length;
}

bool H264FileSource::GetNextFrame(AVPacket *packet) {
    if (!file_) {
        return false;
    }

    std::unique_ptr<H264Nalu> nalu(new H264Nalu);
    ::bzero(nalu.get(), sizeof(H264Nalu));

    int buffersize = 100000;
    nalu->max_size = buffersize;
    nalu->buf.reset(new unsigned char[buffersize]);

    int data_lenth = GetNextNALU(nalu.get());
    if (data_lenth > 0) {
        packet->size = nalu->len;
        packet->buffer = nalu->buf;
        packet->prepend_size = nalu->prepend_size;
        packet->type = nalu->nal_unit_type;

        return true;
    }

    return false;
}

} // namespace muduo_media