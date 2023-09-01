#ifndef A2B03B7D_F0DE_4FAF_BE57_751F96E7384B
#define A2B03B7D_F0DE_4FAF_BE57_751F96E7384B

namespace rtsp {

struct RtspUrl {
    std::string entire;
    std::string prefix;
    std::string host;
    int port = 0;
    std::string session; // suffix
};

struct RtspRequestHeaderFirstLine {
    std::string method;
    std::string version;
    RtspUrl url;
};

struct RtspRequestHeader : public RtspRequestHeaderFirstLine {
    int cseq = 0;
};

struct RespRequestOptions : public RtspRequestHeader {
    std::string user_agent;
};

} // namespace rtsp

#endif /* A2B03B7D_F0DE_4FAF_BE57_751F96E7384B */
