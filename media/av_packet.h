#ifndef BEA8DF55_A35C_4210_A032_E4E70144E119
#define BEA8DF55_A35C_4210_A032_E4E70144E119

#include <memory>
#include <stdint.h>
#include <string>

namespace muduo_media {

struct AVPacket {
    std::shared_ptr<uint8_t[]> buffer; /* 帧数据 */
    uint32_t size = 0;                 /* 帧大小 */
    uint8_t type = 0;                  /* 帧类型 */
    uint32_t timestamp = 0;            /* 时间戳 */
};

} // namespace muduo_media

#endif /* BEA8DF55_A35C_4210_A032_E4E70144E119 */
