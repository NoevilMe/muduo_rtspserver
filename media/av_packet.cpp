#include "av_packet.h"

#include <cstring>

namespace muduo_media {

void H264Nalu::AppendData(unsigned char *data_buf, size_t data_len) {
    if (max_size - len - prepend_size >= data_len) { // space is not enough
        memcpy(this->buf.get() + this->len + prepend_size, data_buf, data_len);
        this->len += data_len;
    } else {
        this->max_size = this->max_size << 1;
        std::shared_ptr<unsigned char[]> new_buf(
            new unsigned char[this->max_size]);
        memcpy(new_buf.get() + prepend_size, this->buf.get() + prepend_size,
               this->len);
        this->buf = new_buf;
        this->AppendData(data_buf, data_len);
    }
}

}