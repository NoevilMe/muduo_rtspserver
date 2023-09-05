#include "rtsp_connection.h"
#include "logger/log_stream.h"
#include "logger/logger.h"
#include "media_session.h"
#include "net/tcp_connection.h"
#include "utils.h"

namespace rtsp {

static const char kRtspUrlPrefix[] = "rtsp://";
static constexpr size_t kRtspUrlPrefixLen = 7;
static_assert(strlen(kRtspUrlPrefix) == kRtspUrlPrefixLen);

static const char kRtspVersion[] = "RTSP/1.0";
static constexpr size_t kRtspVersionLen = 8;
static_assert(strlen(kRtspVersion) == kRtspVersionLen);

static const char kRtspCSeq[] = "CSeq:";
static constexpr size_t kRtspCSeqLen = 5;
static_assert(strlen(kRtspCSeq) == kRtspCSeqLen);

static const char kRtspApplicationSdp[] = "application/sdp";

static constexpr int kRtspPort = 554;

inline muduo::log::LogStream &operator<<(muduo::log::LogStream &s,
                                         RtspStatusCode code) {
    std::string code_str(RtspStatusCodeToString(code));
    s.Append(code_str.data(), code_str.length());
    return s;
}

inline muduo::log::LogStream &operator<<(muduo::log::LogStream &s,
                                         RtspMethod method) {
    std::string code_str(RtspMethodToString(method));
    s.Append(code_str.data(), code_str.length());
    return s;
}

RtspConnection::RtspConnection(const muduo::net::TcpConnectionPtr &conn)
    : tcp_conn_(conn) {
    // 消息回调最迟要在tcp connection before_reading_callback 中来设置
    tcp_conn_->set_message_callback(
        std::bind(&RtspConnection::OnMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
}

void RtspConnection::OnMessage(const muduo::net::TcpConnectionPtr conn,
                               muduo::net::Buffer *buf,
                               muduo::event_loop::Timestamp timestamp) {

    std::shared_ptr<RtspRequestHead> header = ParseRequestHeader(buf);
    if (!header) {
        LOG_ERROR << "parse rtsp request header fail";
        conn->Shutdown();
        return;
    }

    if (header->method == RtspMethod::OPTIONS) {
        HandleRequestMethodOptions(buf, header);
    } else if (header->method == RtspMethod::DESCRIBE) {
        HandleRequestMethodDescribe(buf, header);
    } else if (header->method == RtspMethod::SETUP) {
        HandleRequestMethodSetup(buf, header);
    } else {
        LOG_ERROR << "unhandled method " << header->method;
    }

    auto left_data = buf->RetrieveAllAsString();
    LOG_DEBUG << conn->peer_addr().IpPort() << " received: " << left_data;
}

std::shared_ptr<RtspRequestHead>
RtspConnection::ParseRequestHeader(muduo::net::Buffer *buf) {
    const char *first_crlf = buf->FindCRLF();
    if (first_crlf) {
        char method[32] = {0};
        char url[256] = {0};
        char version[16] = {0};

        if (sscanf(buf->Peek(), "%s %s %s", method, url, version) != 3) {
            LOG_ERROR << "Invalid RTSP first line data";
            return nullptr;
        }

        if (strncmp(url, kRtspUrlPrefix, kRtspUrlPrefixLen) != 0) {
            LOG_ERROR << "rtsp url " << url << " does not start with rtsp://";
            return nullptr;
        }

        if (strncmp(version, kRtspVersion, kRtspVersionLen) != 0) {
            LOG_ERROR << "unsupported rtsp version " << version;
            return nullptr;
        }

        auto rtsp_method = StringToRtspMethod(method);
        if (rtsp_method == RtspMethod::NONE) {
            LOG_ERROR << "unknown method " << method;
            return nullptr;
        }

        std::shared_ptr<RtspRequestHead> req_header(new RtspRequestHead());
        req_header->method = rtsp_method;
        req_header->version.assign(version);
        req_header->url.entire = url;

        uint16_t port = 0;
        char host[64] = {0};
        char suffix[128] = {0};

        char *s_point = url + strlen(kRtspUrlPrefix);
        if (sscanf(s_point, "%[^:]:%hu/%s", host, &port, suffix) == 3) {
            req_header->url.host = host;
            req_header->url.port = port;
            req_header->url.session = suffix;
        } else if (sscanf(s_point, "%[^/]/%s", host, suffix) == 2) {
            req_header->url.host = host;
            req_header->url.port = kRtspPort;
            req_header->url.session = suffix;
            port = 554;
        } else {
            LOG_ERROR << "analyze rtsp " << url << " fail ";
            return nullptr;
        }

        // 第一行处理完毕
        buf->RetrieveUntil(first_crlf + 2);

        if (!ParseCSeq(buf, req_header->cseq)) {
            LOG_ERROR << "parse cseq fail";
            return nullptr;
        }

        LOG_DEBUG << "rtsp method: " << RtspMethodToString(req_header->method)
                  << ", version: " << req_header->version
                  << ", url: " << req_header->url.entire
                  << " (host: " << req_header->url.host
                  << ", port:" << req_header->url.port
                  << ", session: " << req_header->url.session << "), "
                  << "CSeq: " << req_header->cseq;
        return req_header;
    } else {
        return nullptr;
    }
}

bool RtspConnection::ParseCSeq(muduo::net::Buffer *buf, int &cseq) {
    const char *first_crlf = buf->FindCRLF();
    if (first_crlf) {
        std::string line(buf->Peek(), first_crlf);
        buf->RetrieveUntil(first_crlf + 2);

        auto pos = line.find(kRtspCSeq);
        if (pos == std::string ::npos) {
            return false;
        }

        return sscanf(line.data() + pos, "%*[^:]: %d", &cseq) == 1;
    } else {
        return false;
    }
}

void RtspConnection::DiscardAllData(muduo::net::Buffer *buf) {
    LOG_DEBUG << "discard left data: " << buf->RetrieveAllAsString();
}

void RtspConnection::HandleRequestMethodOptions(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;

    auto session = get_media_session_callback_
                       ? get_media_session_callback_(header->url.session)
                       : nullptr;
    if (session) {
        resp_header.code = RtspStatusCode::OK;

        char send_buf[200] = {0};
        int data_size = snprintf(
            send_buf, sizeof(send_buf),
            "%s %d %s\r\n"
            "CSeq: %d\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, TRARDOWN, PLAY\r\n"
            "\r\n",
            resp_header.version.data(), (int)resp_header.code,
            RtspStatusCodeToString(resp_header.code), resp_header.cseq);

        LOG_DEBUG << "response data: " << send_buf << ", size " << data_size;

        tcp_conn_->Send(send_buf, data_size);
        active_media_session_ = session;

    } else {
        resp_header.code = RtspStatusCode::NotAcceptable;
        SendShortResponse(resp_header);
    }
}

void RtspConnection::HandleRequestMethodDescribe(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {

    std::string accept_application_type;

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        // LOG_DEBUG << "line data: " << line << " length " << line.length();

        if (utils::StartsWith(line, "Accept: ")) {
            accept_application_type = line.substr(strlen("Accept: "));
            LOG_DEBUG << "accept type " << accept_application_type;
        } else if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        }
    }

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;

    if (accept_application_type != "application/sdp") {
        resp_header.code = RtspStatusCode::UnsupportedMediaType;
        SendShortResponse(resp_header);
    } else {
        MediaSessionPtr session = active_media_session_.lock();
        if (!session) {
            resp_header.code = RtspStatusCode::NotAcceptable;
            SendShortResponse(resp_header);
        } else {
            resp_header.code = RtspStatusCode::OK;
            std::string sdp = session->BuildSdp();
            char data_buf[1000] = {0};
            int data_size =
                snprintf(data_buf, sizeof(data_buf),
                         "%s %d %s\r\n"
                         "CSeq: %d\r\n"
                         "Content-Length: %lu\r\n"
                         "Content-Type: application/sdp\r\n"
                         "\r\n"
                         "%s",
                         resp_header.version.data(), (int)resp_header.code,
                         RtspStatusCodeToString(resp_header.code),
                         resp_header.cseq, sdp.length(), sdp.data());

            LOG_DEBUG << "response data: " << data_buf << ", size "
                      << data_size;

            tcp_conn_->Send(data_buf, data_size);
        }
    }
}

void RtspConnection::HandleRequestMethodSetup(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {

    std::string protocol;
    std::string cast;
    int send_port = 0;
    int recv_port = 0;

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        // LOG_DEBUG << "line data: " << line << " length " << line.length();

        if (utils::StartsWith(line, "Transport: ")) {
            std::string transport = line.substr(strlen("Transport: "));

            char pro_buf[20] = {0};
            char cast_buf[20] = {0};

            sscanf(transport.data(), "%[^;];%[^;];client_port=%d-%d", pro_buf,
                   cast_buf, &send_port, &recv_port);

            LOG_DEBUG << "transport " << transport << ", protocol " << pro_buf
                      << ", cast " << cast_buf << ", send port " << send_port
                      << ", recv port " << recv_port;
        } else if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        }
    }
}

std::string RtspConnection::ShortResponseMessage(const std::string &version,
                                                 RtspStatusCode code,
                                                 int cseq) {
    char buf[200] = {0};
    int size =
        snprintf(buf, 200,
                 "%s %d %s\r\n"
                 "CSeq: %d\r\n"
                 "\r\n",
                 version.data(), (int)code, RtspStatusCodeToString(code), cseq);

    return std::string(buf, size);
}

void RtspConnection::SendShortResponse(const std::string &version,
                                       RtspStatusCode code, int cseq) {
    char buf[200] = {0};
    int size =
        snprintf(buf, 200,
                 "%s %d %s\r\n"
                 "CSeq: %d\r\n"
                 "\r\n",
                 version.data(), (int)code, RtspStatusCodeToString(code), cseq);
    tcp_conn_->Send(buf, size);
}

void RtspConnection::SendShortResponse(const RtspResponseHead &resp_header) {
    SendShortResponse(resp_header.version, resp_header.code, resp_header.cseq);
}

} // namespace rtsp