#include "rtsp_connection.h"
#include "logger/log_stream.h"
#include "logger/logger.h"
#include "media_session.h"
#include "net/tcp_connection.h"
#include "rtsp_session.h"
#include "utils.h"

namespace muduo_media {

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

RtspConnection::RtspConnection(const muduo::net::TcpConnectionPtr &conn,
                               const GetMediaSessionCallback &cb)
    : tcp_conn_(conn), get_media_session_callback_(cb) {
    // 消息回调最迟要在tcp connection before_reading_callback 中来设置
    tcp_conn_->set_message_callback(
        std::bind(&RtspConnection::OnMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));

    LOG_DEBUG << "RtspConnection::ctor[" << tcp_conn_->name() << "] at "
              << this;
}

RtspConnection::~RtspConnection() {
    LOG_DEBUG << "RtspConnection::dtor[" << tcp_conn_->name() << "] at "
              << this;

    // tcp_conn_.reset();
    rtsp_session_.reset();
}

void RtspConnection::OnMessage(const muduo::net::TcpConnectionPtr conn,
                               muduo::net::Buffer *buf,
                               muduo::event_loop::Timestamp timestamp) {

    std::string data = buf->TryRetrieveAllAsString();
    LOG_DEBUG << "try receive data [" << data << "]";

    std::vector<std::string> gap_lines;

    std::shared_ptr<RtspRequestHead> header = ParseRequestHead(buf, gap_lines);
    if (!header) {
        LOG_ERROR << "parse rtsp request header fail";
        conn->Shutdown();
        return;
    }

    if (header->method == RtspMethod::OPTIONS) {
        HandleMethodOptions(buf, header);
    } else if (header->method == RtspMethod::DESCRIBE) {
        HandleMethodDescribe(buf, header, gap_lines);
    } else if (header->method == RtspMethod::SETUP) {
        HandleMethodSetup(buf, header, gap_lines);
    } else if (header->method == RtspMethod::PLAY) {
        HandleMethodPlay(buf, header);
    } else if (header->method == RtspMethod::TEARDOWN) {
        HandleMethodTeardown(buf, header);
    } else {
        LOG_ERROR << "unhandled method " << header->method;
    }

    auto left_data = buf->RetrieveAllAsString();
    LOG_DEBUG << conn->peer_addr().IpPort() << " received [" << left_data
              << "]";
}

std::shared_ptr<RtspRequestHead>
RtspConnection::ParseRequestHead(muduo::net::Buffer *buf,
                                 std::vector<std::string> &gap_lines) {
    const char *first_crlf = buf->FindCRLF();
    if (first_crlf) {
        char method[32] = {0};
        char url[256] = {0};
        char version[16] = {0};

        if (sscanf(buf->Peek(), "%s %s %s", method, url, version) != 3) {
            LOG_ERROR << "Invalid RTSP reques line data";
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
        LOG_INFO << "rtsp method: " << req_header->method
                 << ", version: " << req_header->version
                 << ", url: " << req_header->url.entire
                 << " (host: " << req_header->url.host
                 << ", port:" << req_header->url.port
                 << ", session: " << req_header->url.session << ")";

        std::string line;
        while (buf->RetrieveCRLFLine(false, line)) {
            if (utils::StartsWith(line, kRtspCSeq)) {
                sscanf(line.data(), "%*[^:]: %d", &req_header->cseq);
                break;
            } else {
                gap_lines.push_back(line);
                LOG_DEBUG << "gap line [" << line << "]";
            }
        }

        if (req_header->cseq == 0) {
            LOG_ERROR << "parse CSeq fail";
            return nullptr;
        } else {
            LOG_INFO << "CSeq: " << req_header->cseq;
            return req_header;
        }
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

void RtspConnection::HandleMethodOptions(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;

    auto session = get_media_session_callback_(header->url.session);
    if (session) {
        active_media_session_ = session;
        media_session_name_ = session->name();

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

        SendResponse(send_buf, data_size);
    } else {
        resp_header.code = RtspStatusCode::NotAcceptable;
        SendShortResponse(resp_header);
    }
}

void RtspConnection::HandleMethodDescribe(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header,
    const std::vector<std::string> &gap_lines) {

    // 如果没有执行OPTIONS
    if (active_media_session_.expired()) {
        auto session = get_media_session_callback_(header->url.session);
        if (session) {
            active_media_session_ = session;
            media_session_name_ = session->name();
        }
    }

    std::string accept_application_type;
    for (auto &&line : gap_lines) {
        if (utils::StartsWith(line, "Accept: ")) {
            accept_application_type = line.substr(strlen("Accept: "));
            LOG_DEBUG << "accept type " << accept_application_type;
        }
    }

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        // LOG_DEBUG << "line data: " << line << " length " <<
        // line.length();

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

            SendResponse(data_buf, data_size);
        }
    }
}

void RtspConnection::HandleMethodSetup(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header,
    const std::vector<std::string> &gap_lines) {

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;
    resp_header.code = RtspStatusCode::OK;

    // TODO: 如果没有OPTIONS、DESCRIBE，直接SETUP
    assert(!media_session_name_.empty());
    std::string session_head = media_session_name_ + "/";
    assert(utils::StartsWith(header->url.session, session_head));

    std::string track = header->url.session.substr(session_head.size());
    LOG_DEBUG << "session track " << track;

    auto media_session = active_media_session_.lock();
    if (!media_session) {
        LOG_ERROR << "media session is null";
        resp_header.code = RtspStatusCode::NotFound;
        SendShortResponse(resp_header);
        return;
    }

    if (!media_session->SubsessionExists(track)) {
        LOG_ERROR << "can not find subsession " << track;
        resp_header.code = RtspStatusCode::NotFound;
        SendShortResponse(resp_header);
        return;
    }

    unsigned short rtp_port = 0;
    unsigned short rtcp_port = 0;

    auto line_handler([&, this](const std::string &line) {
        if (utils::StartsWith(line, "Transport: ")) {
            std::string transport = line.substr(strlen("Transport: "));

            char pro_buf[20] = {0};
            char cast_buf[20] = {0};

            if (sscanf(transport.data(), "%[^;];%[^;];client_port=%hu-%hu",
                       pro_buf, cast_buf, &rtp_port, &rtcp_port) == 4) {

                std::string protocol(pro_buf);
                std::string cast(cast_buf);

                LOG_DEBUG << "transport " << transport << ", media protocol "
                          << protocol << ", " << cast_buf << ", rtp port "
                          << rtp_port << ", rtcp port " << rtcp_port;

                auto peer_ip = tcp_conn_->peer_addr().Ip();
                muduo::net::InetAddress peer_rtp_addr(peer_ip, rtp_port);
                muduo::net::InetAddress peer_rtcp_addr(peer_ip, rtcp_port);

                unsigned short local_rtp_port;
                unsigned short local_rtcp_port;

                if (!rtsp_session_) {
                    rtsp_session_.reset(new RtspSession(tcp_conn_->loop(),
                                                        active_media_session_));
                }

                rtsp_session_->Setup(track, peer_rtp_addr, peer_rtcp_addr,
                                     local_rtp_port, local_rtcp_port);

                auto session_id = rtsp_session_->id();
                LOG_DEBUG << "local rtp port " << local_rtp_port
                          << ", rtcp port " << local_rtcp_port
                          << ", session id " << session_id;

                char send_buf[300] = {0};
                int size =
                    snprintf(send_buf, sizeof(send_buf),
                             "%s %d %s\r\n"
                             "CSeq: %d\r\n"
                             "Transport: %s;server_port=%hu-%hu\r\n"
                             "Session: %d\r\n"
                             "\r\n",
                             resp_header.version.data(), (int)resp_header.code,
                             RtspStatusCodeToString(resp_header.code),
                             resp_header.cseq, transport.data(), local_rtp_port,
                             local_rtcp_port, session_id);
                SendResponse(send_buf, size);
            } else {
                LOG_ERROR << "unsupported setup params";
            }

        } else if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        }
    });

    for (auto &line : gap_lines) {
        line_handler(line);
    }

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        // LOG_DEBUG << "line data: " << line << " length " <<
        // line.length();
        line_handler(line);
    }
}

void RtspConnection::HandleMethodPlay(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        } else if (utils::StartsWith(line, "Session: ")) {
            LOG_DEBUG << "session " << line.substr(strlen("Session: "));
        }
    }
    rtsp_session_->Play();

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;
    resp_header.code = RtspStatusCode::OK;

    char send_buf[300] = {0};
    auto data_len = snprintf(send_buf, sizeof(send_buf),
                             "%s %d %s\r\n"
                             "CSeq: %d\r\n"
                             "Range: npt=0.000-\r\n"
                             "Session: %d; timeout=60\r\n",
                             resp_header.version.data(), (int)resp_header.code,
                             RtspStatusCodeToString(resp_header.code),
                             resp_header.cseq, rtsp_session_->id());

    SendResponse(send_buf, data_len);
}

void RtspConnection::HandleMethodTeardown(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &header) {
    rtsp_session_->Teardown();
    rtsp_session_.reset();

    RtspResponseHead resp_header;
    resp_header.version = header->version;
    resp_header.cseq = header->cseq;
    resp_header.code = RtspStatusCode::OK;
    SendShortResponse(resp_header);
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
    SendResponse(buf, size);
}

void RtspConnection::SendShortResponse(const RtspResponseHead &resp_header) {
    SendShortResponse(resp_header.version, resp_header.code, resp_header.cseq);
}

void RtspConnection::SendResponse(const char *buf, int size) {
    LOG_DEBUG << "size " << size << " -\r\n" << muduo::StringPiece(buf, size);
    tcp_conn_->Send(buf, size);
}

} // namespace muduo_media