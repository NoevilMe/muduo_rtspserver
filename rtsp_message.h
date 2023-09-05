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
    PAUSE,
    RECORD,
    ANNOUNCE,
    TEARDOWN,
    REDIRECT,
    GET_PARAMETER,
    SET_PARAMETER,
    NONE
};

struct RtspRequestLine {
    RtspMethod method;
    std::string version;
    RtspUrl url;
};

struct RtspRequestHead : public RtspRequestLine {
    int cseq = 0;
};

struct RespRequestOptions : public RtspRequestHead {
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

struct RtspStatusLine {
    std::string version;
    RtspStatusCode code;
}; // The first line of a Response message is the Status-Line

struct RtspResponseHead : public RtspStatusLine {
    int cseq = 0;
};

} // namespace rtsp

#endif /* A2B03B7D_F0DE_4FAF_BE57_751F96E7384B */
