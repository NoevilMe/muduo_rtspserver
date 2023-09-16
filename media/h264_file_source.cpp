#include "h264_file_source.h"
#include "media/av_packet.h"

#include <cstring>

namespace muduo_media {

typedef enum {
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,
} H264NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} H264NaluPriority;

void H264Nalu::AppendData(unsigned char *data_buf, size_t data_len) {
    if (max_size - len >= data_len) { // space is not enough
        memcpy(this->buf + this->len, data_buf, data_len);
        this->len += data_len;
    } else {
        this->max_size = this->max_size << 1;
        unsigned char *new_buf = new unsigned char[this->max_size];
        memcpy(new_buf, this->buf, this->len);
        delete[] this->buf;
        this->buf = new_buf;
        this->AppendData(data_buf, data_len);
    }
}

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

H264FileSource::H264FileSource(FILE *file) : MultiFrameFileSource(file) {}

H264FileSource::~H264FileSource() {}

int H264FileSource::GetAnnexbNALU(H264Nalu *nalu) {
    nalu->len = 0; // reset immediately
    nalu->statcode_length = 0;

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
                printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
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
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
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

    nalu->forbidden_bit = nalu->buf[0] & 0x80;     // 1 bit
    nalu->nal_reference_bit = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   // 5 bit

    return nalu->len + nalu->statcode_length;
}

bool H264FileSource::GetNextFrame(AVPacket *packet) {
    if (!file_) {
        return false;
    }

    H264Nalu *n = new H264Nalu;
    int buffersize = 100000;

    n->max_size = buffersize;
    n->buf = new unsigned char[buffersize];

    int data_lenth = GetAnnexbNALU(n);
    if (data_lenth > 0) {
        packet->size = n->len;
        packet->buffer.reset(n->buf);
    }

    delete n;

    return data_lenth > 0;
}

} // namespace muduo_media