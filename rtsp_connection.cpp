#include "rtsp_connection.h"
#include "logger/logger.h"
#include "net/tcp_connection.h"

static const char kRtspUrlPrefix[] = "rtsp://";
static constexpr size_t kkRtspUrlPrefixLen = 7;
static const char kRtspVersion[] = "RTSP/1.0";
static constexpr size_t kRtspVersionLen = 8;
static const char kRtspCSeq[] = "CSeq:";
static constexpr size_t kRtspCSeqLen = 5;
static constexpr int kRtspPort = 554;

RtspConnection::RtspConnection(const muduo::net::TcpConnectionPtr &conn)
    : tcp_conn_(conn) {

    // 直接在连接上设置消息回调

    tcp_conn_->set_message_callback(
        std::bind(&RtspConnection::OnMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
}

void RtspConnection::OnMessage(const muduo::net::TcpConnectionPtr conn,
                               muduo::net::Buffer *buf,
                               muduo::event_loop::Timestamp timestamp) {

    std::shared_ptr<RtspRequestHeader> request_header = ParseRequestHeader(buf);
    if (!request_header) {
        LOG_ERROR << "parse rtsp request header fail";
        DiscardAllData(buf);
    }

    int cseq = 0;
    if (!ParseCSeq(buf, cseq)) {
        LOG_ERROR << "parse cseq fail";
    }

    LOG_DEBUG << "cseq " << cseq;

    LOG_DEBUG << conn->peer_addr().IpPort()
              << " received: " << buf->RetrieveAllAsString();
}

std::shared_ptr<RtspRequestHeader>
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

        if (strncmp(url, kRtspUrlPrefix, strlen(kRtspUrlPrefix)) != 0) {
            LOG_ERROR << "rtsp url " << url << " does not start with rtsp://";
            return nullptr;
        }

        if (strncmp(version, kRtspVersion, strlen(kRtspVersion)) != 0) {
            LOG_ERROR << "unsupported rtsp version " << version;
            return nullptr;
        }

        std::shared_ptr<RtspRequestHeader> req_header(new RtspRequestHeader());
        req_header->method.assign(method);
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

        LOG_DEBUG << "rtsp method: " << req_header->method
                  << ", version: " << req_header->version
                  << ", url: " << req_header->url.entire
                  << " (host: " << req_header->url.host
                  << ", port:" << req_header->url.port
                  << ", session: " << req_header->url.session << ")";
        buf->RetrieveUntil(first_crlf + 2);
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
