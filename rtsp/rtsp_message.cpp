#include "rtsp_message.h"

namespace muduo_media {

struct RtspLabel {
    int value;
    const char *name;
};

static RtspLabel s_rtsp_method_labels[] = {
    {(int)RtspMethod::OPTIONS, "OPTIONS"},
    {(int)RtspMethod::DESCRIBE, "DESCRIBE"},
    {(int)RtspMethod::SETUP, "SETUP"},
    {(int)RtspMethod::PLAY, "PLAY"},
    {(int)RtspMethod::PAUSE, "PAUSE"},
    {(int)RtspMethod::RECORD, "RECORD"},
    {(int)RtspMethod::ANNOUNCE, "ANNOUNCE"},
    {(int)RtspMethod::TEARDOWN, "TEARDOWN"},
    {(int)RtspMethod::REDIRECT, "REDIRECT"},
    {(int)RtspMethod::GET_PARAMETER, "GET_PARAMETER"},
    {(int)RtspMethod::SET_PARAMETER, "SET_PARAMETER"},
    {-1, nullptr}};

const char *RtspMethodToString(RtspMethod method) {
    RtspLabel *cur = s_rtsp_method_labels;
    while (cur->name) {
        if (cur->value == (int)method) {
            return cur->name;
        }
        ++cur;
    }

    return "NONE";
}

RtspMethod StringToRtspMethod(const std::string &str) {
    RtspLabel *cur = s_rtsp_method_labels;
    while (cur->name) {
        if (str == cur->name) {
            return (RtspMethod)cur->value;
        }
        ++cur;
    }

    return RtspMethod::NONE;
}

// https://www.websitepulse.com/kb/rtsp_status_codes
// https://www.ietf.org/rfc/rfc2326.txt
/**
 * @brief
RFC 2326              Real Time Streaming Protocol            April 1998

   Status-Code  =     "100"      ; Continue
                |     "200"      ; OK
                |     "201"      ; Created
                |     "250"      ; Low on Storage Space
                |     "300"      ; Multiple Choices
                |     "301"      ; Moved Permanently
                |     "302"      ; Moved Temporarily
                |     "303"      ; See Other
                |     "304"      ; Not Modified
                |     "305"      ; Use Proxy
                |     "400"      ; Bad Request
                |     "401"      ; Unauthorized
                |     "402"      ; Payment Required
                |     "403"      ; Forbidden
                |     "404"      ; Not Found
                |     "405"      ; Method Not Allowed
                |     "406"      ; Not Acceptable
                |     "407"      ; Proxy Authentication Required
                |     "408"      ; Request Time-out
                |     "410"      ; Gone
                |     "411"      ; Length Required
                |     "412"      ; Precondition Failed
                |     "413"      ; Request Entity Too Large
                |     "414"      ; Request-URI Too Large
                |     "415"      ; Unsupported Media Type
                |     "451"      ; Parameter Not Understood
                |     "452"      ; Conference Not Found
                |     "453"      ; Not Enough Bandwidth
                |     "454"      ; Session Not Found
                |     "455"      ; Method Not Valid in This State
                |     "456"      ; Header Field Not Valid for Resource
                |     "457"      ; Invalid Range
                |     "458"      ; Parameter Is Read-Only
                |     "459"      ; Aggregate operation not allowed
                |     "460"      ; Only aggregate operation allowed
                |     "461"      ; Unsupported transport
                |     "462"      ; Destination unreachable
                |     "500"      ; Internal Server Error
                |     "501"      ; Not Implemented
                |     "502"      ; Bad Gateway
                |     "503"      ; Service Unavailable
                |     "504"      ; Gateway Time-out
                |     "505"      ; RTSP Version not supported
                |     "551"      ; Option not supported
                |     extension-code
 */

static RtspLabel s_status_code_names[] = {
    {(int)RtspStatusCode::OK, "OK"},
    {(int)RtspStatusCode::BadRequest, "Bad Request"},
    {(int)RtspStatusCode::Unauthorized, "Unauthorized"},
    {(int)RtspStatusCode::Forbidden, "Forbidden"},
    {(int)RtspStatusCode::NotFound, "Not Found"},
    {(int)RtspStatusCode::MethodNotAllowed, "Method Not Allowed"},
    {(int)RtspStatusCode::NotAcceptable, "Not Acceptable"},
    {(int)RtspStatusCode::UnsupportedMediaType, "Unsupported Media Type"},
    {(int)RtspStatusCode::SessionNotFound, "Session Not Found"},
    {(int)RtspStatusCode::UnsupportedTransport, "Unsupported transport"},
    {(int)RtspStatusCode::None, nullptr}};

const char *RtspStatusCodeToString(RtspStatusCode code) {
    RtspLabel *cur = s_status_code_names;
    while (cur->name) {
        if (cur->value == (int)code) {
            return cur->name;
        }
        ++cur;
    }

    return "";
}

} // namespace muduo_media