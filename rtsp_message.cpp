#include "rtsp_message.h"

namespace rtsp {

const char *RtspMethodToString(RtspMethod method) {
    switch (method) {
    case RtspMethod::OPTIONS:
        return "OPTIONS";
    case RtspMethod::DESCRIBE:
        return "DESCRIBE";
    case RtspMethod::SETUP:
        return "SETUP";
    case RtspMethod::PLAY:
        return "PLAY";
    case RtspMethod::TEARDOWN:
        return "TEARDOWN";
    case RtspMethod::GET_PARAMETER:
        return "GET_PARAMETER";
    case RtspMethod::RTCP:
        return "RTCP";
    default:
        return "NONE";
    }
}

RtspMethod StringToRtspMethod(const std::string &str) {
    if (str == "OPTIONS") {
        return RtspMethod::OPTIONS;
    } else if (str == "DESCRIBE") {
        return RtspMethod::DESCRIBE;
    } else if (str == "SETUP") {
        return RtspMethod::SETUP;
    } else if (str == "PLAY") {
        return RtspMethod::PLAY;
    } else if (str == "TEARDOWN") {
        return RtspMethod::TEARDOWN;
    } else if (str == "GET_PARAMETER") {
        return RtspMethod::GET_PARAMETER;
    } else if (str == "RTCP") {
        return RtspMethod::RTCP;
    } else {
        return RtspMethod::NONE;
    }
}

// https://www.websitepulse.com/kb/rtsp_status_codes
/**
 * @brief
100 - Continue
200 - OK
201 - Created
250 - Low on Storage Space
300 - Multiple Choices
301 - Moved Permanently
302 - Moved Temporarily
303 - See Other
304 - Not Modified
305 - Use Proxy
400 - Bad Request
401 - Unauthorized
402 - Payment Required
403 - Forbidden
404 - Not Found
405 - Method Not Allowed
406 - Not Acceptable
407 - Proxy Authentication Required
408 - Request Time-out
410 - Gone
411 - Length Required
412 - Precondition Failed
413 - Request Entity Too Large
414 - Request-URI Too Large
415 - Unsupported Media Type
451 - Parameter Not Understood
452 - Conference Not Found
453 - Not Enough Bandwidth
454 - Session Not Found
455 - Method Not Valid in This State
456 - Header Field Not Valid for Resource
457 - Invalid Range
458 - Parameter Is Read-Only
459 - Aggregate operation not allowed
460 - Only aggregate operation allowed
461 - Unsupported transport
462 - Destination unreachable
463 - Key management Failure
500 - Internal Server Error
501 - Not Implemented
502 - Bad Gateway
503 - Service Unavailable
504 - Gateway Time-out
505 - RTSP Version not supported
551 - Option not supported
 */

struct RtspStatusCodeLabel {
    RtspStatusCode code;
    const char *name;
};

static RtspStatusCodeLabel s_status_code_names[] = {
    {RtspStatusCode::OK, "OK"},
    {RtspStatusCode::BadRequest, "Bad Request"},
    {RtspStatusCode::Unauthorized, "Unauthorized"},
    {RtspStatusCode::Forbidden, "Forbidden"},
    {RtspStatusCode::NotFound, "Not Found"},
    {RtspStatusCode::MethodNotAllowed, "Method Not Allowed"},
    {RtspStatusCode::NotAcceptable, "Not Acceptable"},
    {RtspStatusCode::UnsupportedMediaType, "Unsupported Media Type"},
    {RtspStatusCode::SessionNotFound, "Session Not Found"},
    {RtspStatusCode::None, nullptr}};

const char *RtspStatusCodeToString(RtspStatusCode code) {
    RtspStatusCodeLabel *cur = s_status_code_names;
    while (cur->name) {
        if (cur->code == code) {
            return cur->name;
        }
        ++cur;
    }

    return "";
}

} // namespace rtsp