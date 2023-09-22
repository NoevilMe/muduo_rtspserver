#include "rtsp_connection.h"
#include "eventloop/endian.h"
#include "logger/log_stream.h"
#include "logger/logger.h"
#include "media/rtcp.h"
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

static constexpr int kRtspPort = 554;

/************************ logger helper ***************************/
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

/*=====================================================================*/
RtspConnection::RtspConnection(const muduo::net::TcpConnectionPtr &conn,
                               const GetMediaSessionCallback &cb)
    : tcp_conn_(conn),
      get_media_session_callback_(cb),
      next_type_(kMessageNone),
      next_length_(0),
      rtp_transport_(RtpTransportProtocol::kRtpTransportNone),
      rtp_channel_(-1),
      rtcp_channel_(-1) {

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

    tcp_conn_.reset();
    rtsp_session_.reset();
}

void RtspConnection::OnMessage(const muduo::net::TcpConnectionPtr conn,
                               muduo::net::Buffer *buf,
                               muduo::event_loop::Timestamp timestamp) {

    LOG_TRACE << "available bytes " << buf->ReadableBytes();

    std::string data = buf->TryRetrieveAllAsString();
    LOG_TRACE << "try receive data [" << data << "]";

    auto data_ptr = buf->Peek();
    if (*data_ptr == kRtspInterleavedFrameMagic) {
        if (buf->ReadableBytes() < sizeof(RtspInterleavedFrame)) {
            LOG_ERROR << "RTSP Interleaved Frame packet size must be >="
                      << sizeof(RtspInterleavedFrame);
            // 继续接收数据
            return;
        } else {
            LOG_TRACE << "parse RTSP Interleaved Frame";
            RtspInterleavedFrame rif{0};
            memcpy(&rif, data_ptr, sizeof(RtspInterleavedFrame));
            rif.length = muduo::NetworkToHost16(rif.length);
            LOG_DEBUG << "RTSP Interleaved Frame, magic " << (char)rif.magic
                      << ", channel " << rif.channel << ", length "
                      << rif.length;

            if (rif.channel == rtp_channel_) {
                LOG_DEBUG << "rtp channel " << rif.channel;
                next_type_ = kMessageRtp;
            } else if (rif.channel == rtcp_channel_) {
                LOG_DEBUG << "rtcp channel " << rif.channel;
                next_type_ = kMessageRtcp;
                next_length_ = rif.length;
            }
            buf->Retrieve(sizeof(RtspInterleavedFrame));
            return;
        }
    } else if (kMessageRtcp == next_type_) {
        if (buf->ReadableBytes() < next_length_)
            return;

        LOG_DEBUG << "parse RTCP";
        RtcpHeader rtcp = {0};
        memcpy(&rtcp, data_ptr, sizeof(RtcpHeader));
        rtcp.length = muduo::NetworkToHost16(rtcp.length);

        LOG_DEBUG << "rtcp V " << rtcp.v << ", P " << rtcp.p << ", RC "
                  << rtcp.rc << ", PT " << rtcp.pt << ", length "
                  << rtcp.length;
        // buf->Retrieve(sizeof(RtcpHeader));
        buf->Retrieve(next_length_);
        LOG_DEBUG << "RTCP retrieve " << next_length_;
        next_length_ = 0;
        next_type_ = kMessageNone;
        return;
    }

    // Request line 与 CSeq之间可能存在数据——ffmpeg请求会有这个问题。
    std::vector<std::string> gap_lines;

    std::shared_ptr<RtspRequestHead> request_head =
        ParseRequestHead(buf, gap_lines);

    if (!request_head) {
        LOG_ERROR << "parse rtsp request header fail";
        conn->Shutdown();
        return;
    }

    if (request_head->method == RtspMethod::OPTIONS) {
        HandleMethodOptions(buf, request_head);
    } else if (request_head->method == RtspMethod::DESCRIBE) {
        HandleMethodDescribe(buf, request_head, gap_lines);
    } else if (request_head->method == RtspMethod::SETUP) {
        HandleMethodSetup(buf, request_head, gap_lines);
    } else if (request_head->method == RtspMethod::PLAY) {
        HandleMethodPlay(buf, request_head);
    } else if (request_head->method == RtspMethod::TEARDOWN) {
        HandleMethodTeardown(buf, request_head);
    } else {
        LOG_ERROR << "unhandled method " << request_head->method;
    }

    if (buf->ReadableBytes() > 0) {
        auto left_data = buf->RetrieveAllAsString();
        LOG_DEBUG << conn->peer_addr().IpPort() << " received left data ["
                  << left_data << "]";
    }
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

        std::shared_ptr<RtspRequestHead> req_head(new RtspRequestHead());
        req_head->method = rtsp_method;
        req_head->version.assign(version);
        req_head->url.entire = url;

        uint16_t port = 0;
        char host[64] = {0};
        char suffix[128] = {0};

        char *s_point = url + strlen(kRtspUrlPrefix);
        if (sscanf(s_point, "%[^:]:%hu/%s", host, &port, suffix) == 3) {
            req_head->url.host = host;
            req_head->url.port = port;
            req_head->url.session = suffix;
        } else if (sscanf(s_point, "%[^/]/%s", host, suffix) == 2) {
            req_head->url.host = host;
            req_head->url.port = kRtspPort;
            req_head->url.session = suffix;
            port = 554;
        } else {
            LOG_ERROR << "analyze rtsp " << url << " fail ";
            return nullptr;
        }

        // 第一行处理完毕
        buf->RetrieveUntil(first_crlf + 2);
        LOG_INFO << "rtsp method: " << req_head->method
                 << ", version: " << req_head->version
                 << ", url: " << req_head->url.entire
                 << " (host: " << req_head->url.host
                 << ", port:" << req_head->url.port
                 << ", session: " << req_head->url.session << ")";

        std::string line;
        while (buf->RetrieveCRLFLine(false, line)) {
            if (utils::StartsWith(line, kRtspCSeq)) {
                sscanf(line.data(), "%*[^:]: %d", &req_head->cseq);
                break;
            } else {
                gap_lines.push_back(line);
                LOG_DEBUG << "gap line [" << line << "]";
            }
        }

        if (req_head->cseq == 0) {
            LOG_ERROR << "parse CSeq fail";
            return nullptr;
        } else {
            LOG_INFO << "CSeq: " << req_head->cseq;
            return req_head;
        }
    } else {
        return nullptr;
    }
}

void RtspConnection::DiscardAllData(muduo::net::Buffer *buf) {
    auto left_data = buf->RetrieveAllAsString();
    LOG_DEBUG << "discard left data: " << left_data;
}

void RtspConnection::HandleMethodOptions(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &head) {

    RtspResponseHead resp_head;
    resp_head.version = head->version;
    resp_head.cseq = head->cseq;

    auto session = get_media_session_callback_(head->url.session);
    if (session) {
        active_media_session_ = session;
        media_session_name_ = session->name();

        resp_head.code = RtspStatusCode::OK;

        char send_buf[200] = {0};
        int data_size =
            snprintf(send_buf, sizeof(send_buf),
                     "%s %d %s\r\n"
                     "CSeq: %d\r\n"
                     "Public: %s\r\n"
                     "\r\n",
                     resp_head.version.data(), (int)resp_head.code,
                     RtspStatusCodeToString(resp_head.code), resp_head.cseq,
                     session->GetMethodsAsString().data());

        SendResponse(send_buf, data_size);
    } else {
        resp_head.code = RtspStatusCode::NotAcceptable;
        SendShortResponse(resp_head);
    }
}

void RtspConnection::HandleMethodDescribe(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &head,
    const std::vector<std::string> &gap_lines) {

    // 如果没有执行OPTIONS
    if (active_media_session_.expired()) {
        auto session = get_media_session_callback_(head->url.session);
        if (session) {
            active_media_session_ = session;
            media_session_name_ = session->name();
        }
    }

    std::string accept_application_type;

    auto line_handler([&, this](const std::string &line) {
        if (utils::StartsWith(line, "Accept: ")) {
            accept_application_type = line.substr(strlen("Accept: "));
            LOG_DEBUG << "accept type " << accept_application_type;
        } else if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        }
    });

    for (auto &&line : gap_lines) {
        line_handler(line);
    }

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        line_handler(line);
    }

    RtspResponseHead resp_head;
    resp_head.version = head->version;
    resp_head.cseq = head->cseq;

    if (accept_application_type != kRtspApplicationSdp) {
        resp_head.code = RtspStatusCode::UnsupportedMediaType;
        SendShortResponse(resp_head);
    } else {
        MediaSessionPtr session = active_media_session_.lock();
        if (!session) {
            resp_head.code = RtspStatusCode::NotAcceptable;
            SendShortResponse(resp_head);
        } else {
            resp_head.code = RtspStatusCode::OK;
            std::string sdp = session->BuildSdp();

            size_t buf_len = sdp.length() + 100;
            std::unique_ptr<char[]> databuf(new char[buf_len]);
            int data_size =
                snprintf(databuf.get(), buf_len,
                         "%s %d %s\r\n"
                         "CSeq: %d\r\n"
                         "Content-Length: %lu\r\n"
                         "Content-Type: application/sdp\r\n"
                         "\r\n"
                         "%s",
                         resp_head.version.data(), (int)resp_head.code,
                         RtspStatusCodeToString(resp_head.code), resp_head.cseq,
                         sdp.length(), sdp.data());

            SendResponse(databuf.get(), data_size);
        }
    }
}

void RtspConnection::HandleMethodSetup(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &head,
    const std::vector<std::string> &gap_lines) {

    RtspResponseHead resp_head;
    resp_head.version = head->version;
    resp_head.cseq = head->cseq;
    resp_head.code = RtspStatusCode::OK;

    // TODO: 如果没有OPTIONS、DESCRIBE，直接SETUP
    assert(!media_session_name_.empty());
    std::string session_head = media_session_name_ + "/";
    assert(utils::StartsWith(head->url.session, session_head));

    std::string track = head->url.session.substr(session_head.size());
    LOG_DEBUG << "session track " << track;

    auto media_session = active_media_session_.lock();
    if (!media_session) {
        LOG_ERROR << "media session is null";
        resp_head.code = RtspStatusCode::NotFound;
        SendShortResponse(resp_head);
        return;
    }

    if (!media_session->SubsessionExists(track)) {
        LOG_ERROR << "can not find subsession " << track;
        resp_head.code = RtspStatusCode::NotFound;
        SendShortResponse(resp_head);
        return;
    }

    std::string transport;

    auto line_handler([&, this](const std::string &line) {
        if (utils::StartsWith(line, "Transport: ")) {
            transport = line.substr(strlen("Transport: "));
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

    if (transport.empty()) {
        LOG_ERROR << "no transport";
        resp_head.code = RtspStatusCode::UnsupportedTransport;
        SendShortResponse(resp_head);
        return;
    }

    if (transport.find(kRtpTransportProtocolTcp) != std::string::npos) { // tcp

        char protocol_buf[20] = {0};
        char cast_buf[20] = {0};

        unsigned short rtp_channel = 0, rtcp_channel = 0;
        if (sscanf(transport.data(), "%[^;];%[^;];interleaved=%hu-%hu",
                   protocol_buf, cast_buf, &rtp_channel, &rtcp_channel) != 4) {
            LOG_ERROR << "unsupported setup params for transport "
                      << kRtpTransportProtocolTcp;

            resp_head.code = RtspStatusCode::UnsupportedTransport;
            SendShortResponse(resp_head);
            return;
        }

        rtp_transport_ = RtpTransportProtocol::kRtpOverTcp;

        std::string protocol(protocol_buf);
        std::string cast(cast_buf);

        LOG_DEBUG << "transport " << transport << ", transport protocol "
                  << protocol << ", " << cast_buf << ", rtp channel "
                  << rtp_channel << ", rtcp channel " << rtcp_channel;

        if (!rtsp_session_) {
            rtsp_session_.reset(
                new RtspSession(tcp_conn_->loop(), active_media_session_));
        }

        rtsp_session_->Setup(track, tcp_conn_, rtp_channel, rtcp_channel);
        rtp_channel_ = rtp_channel;
        rtcp_channel_ = rtcp_channel;

        auto session_id = rtsp_session_->id();
        LOG_DEBUG << "session id " << session_id;

        char send_buf[300] = {0};
        int size = snprintf(send_buf, sizeof(send_buf),
                            "%s %d %s\r\n"
                            "CSeq: %d\r\n"
                            "Transport: %s\r\n"
                            "Session: %d\r\n"
                            "\r\n",
                            resp_head.version.data(), (int)resp_head.code,
                            RtspStatusCodeToString(resp_head.code),
                            resp_head.cseq, transport.data(), session_id);
        SendResponse(send_buf, size);
    } else if (transport.find(kRtpTransportProtocolUdp) !=
               std::string::npos) { // udp

        unsigned short rtp_port = 0;
        unsigned short rtcp_port = 0;
        char pro_buf[20] = {0};
        char cast_buf[20] = {0};

        if (sscanf(transport.data(), "%[^;];%[^;];client_port=%hu-%hu", pro_buf,
                   cast_buf, &rtp_port, &rtcp_port) != 4) {
            LOG_ERROR << "unsupported setup params for transport "
                      << kRtpTransportProtocolUdp;
            resp_head.code = RtspStatusCode::UnsupportedTransport;
            SendShortResponse(resp_head);
            return;
        }

        rtp_transport_ = RtpTransportProtocol::kRtpOverUdp;

        std::string protocol(pro_buf);
        std::string cast(cast_buf);

        LOG_DEBUG << "transport " << transport << ", media protocol "
                  << protocol << ", " << cast_buf << ", rtp port " << rtp_port
                  << ", rtcp port " << rtcp_port;

        auto peer_ip = tcp_conn_->peer_addr().Ip();
        muduo::net::InetAddress peer_rtp_addr(peer_ip, rtp_port);
        muduo::net::InetAddress peer_rtcp_addr(peer_ip, rtcp_port);

        unsigned short local_rtp_port;
        unsigned short local_rtcp_port;

        if (!rtsp_session_) {
            rtsp_session_.reset(
                new RtspSession(tcp_conn_->loop(), active_media_session_));
        }

        rtsp_session_->Setup(track, peer_rtp_addr, peer_rtcp_addr,
                             local_rtp_port, local_rtcp_port);

        auto session_id = rtsp_session_->id();
        LOG_DEBUG << "local rtp port " << local_rtp_port << ", rtcp port "
                  << local_rtcp_port << ", session id " << session_id;

        char send_buf[300] = {0};
        int size = snprintf(send_buf, sizeof(send_buf),
                            "%s %d %s\r\n"
                            "CSeq: %d\r\n"
                            "Transport: %s;server_port=%hu-%hu\r\n"
                            "Session: %d\r\n"
                            "\r\n",
                            resp_head.version.data(), (int)resp_head.code,
                            RtspStatusCodeToString(resp_head.code),
                            resp_head.cseq, transport.data(), local_rtp_port,
                            local_rtcp_port, session_id);
        SendResponse(send_buf, size);
    } else {
        LOG_ERROR << "unsupported transport protocol";
        resp_head.code = RtspStatusCode::UnsupportedTransport;
        SendShortResponse(resp_head);
        return;
    }
}

void RtspConnection::HandleMethodPlay(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &head) {

    std::string line;
    while (buf->RetrieveCRLFLine(false, line)) {
        if (utils::StartsWith(line, "User-Agent: ")) {
            LOG_DEBUG << "agent " << line.substr(strlen("User-Agent: "));
        } else if (utils::StartsWith(line, "Session: ")) {
            LOG_DEBUG << "session " << line.substr(strlen("Session: "));
        }
    }
    rtsp_session_->Play();

    RtspResponseHead resp_head;
    resp_head.version = head->version;
    resp_head.cseq = head->cseq;
    resp_head.code = RtspStatusCode::OK;

    char send_buf[300] = {0};
    auto data_len = snprintf(send_buf, sizeof(send_buf),
                             "%s %d %s\r\n"
                             "CSeq: %d\r\n"
                             "Range: npt=0.000-\r\n"
                             "Session: %d; timeout=60\r\n"
                             "\r\n",
                             resp_head.version.data(), (int)resp_head.code,
                             RtspStatusCodeToString(resp_head.code),
                             resp_head.cseq, rtsp_session_->id());

    SendResponse(send_buf, data_len);
}

void RtspConnection::HandleMethodTeardown(
    muduo::net::Buffer *buf, const std::shared_ptr<RtspRequestHead> &head) {
    rtsp_session_->Teardown();
    rtsp_session_.reset();

    RtspResponseHead resp_head;
    resp_head.version = head->version;
    resp_head.cseq = head->cseq;
    resp_head.code = RtspStatusCode::OK;
    SendShortResponse(resp_head);
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

void RtspConnection::SendShortResponse(const RtspResponseHead &resp_head) {
    SendShortResponse(resp_head.version, resp_head.code, resp_head.cseq);
}

void RtspConnection::SendResponse(const char *buf, int size) {
    LOG_DEBUG << "size " << size << ", [\r\n"
              << muduo::StringPiece(buf, size) << "]";
    tcp_conn_->Send(buf, size);
}

} // namespace muduo_media