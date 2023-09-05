#ifndef A2B03B7D_F0DE_4FAF_BE57_751F96E7384B
#define A2B03B7D_F0DE_4FAF_BE57_751F96E7384B

#include <string>

namespace rtsp {

struct RtspUrl {
    std::string entire;
    std::string prefix;
    std::string host;
    int port = 0;
    std::string session; // suffix
};

enum class RtspMethod {
    OPTIONS = 0,
    DESCRIBE,
    SETUP,
    PLAY,
    TEARDOWN,
    GET_PARAMETER,
    RTCP,
    NONE
};

struct RtspRequestHeaderFirstLine {
    RtspMethod method;
    std::string version;
    RtspUrl url;
};

struct RtspRequestHeader : public RtspRequestHeaderFirstLine {
    int cseq = 0;
};

struct RespRequestOptions : public RtspRequestHeader {
    std::string user_agent;
};

const char *RtspMethodToString(RtspMethod method);
RtspMethod StringToRtspMethod(const std::string &str);

enum class RtspStatusCode {
    None = 0,
    OK = 200,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    UnsupportedMediaType = 415,
    SessionNotFound = 454
}; // https://www.websitepulse.com/kb/rtsp_status_codes

const char *RtspStatusCodeToString(RtspStatusCode code);

struct RtspResponseHeaderFirstLine {
    std::string version;
    RtspStatusCode code;
};

struct RtspResponseHeader : public RtspResponseHeaderFirstLine {
    int cseq = 0;
};



} // namespace rtsp

#endif /* A2B03B7D_F0DE_4FAF_BE57_751F96E7384B */
