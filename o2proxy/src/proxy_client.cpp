#include <vector>

#include <sys/socket.h> // socket(), AF_INET/PF_INET
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_aton()
#include <netdb.h>      // gethostbyname
#include <fcntl.h>      // F_GETFL, O_NONBLOCK e.t.c.
#include <errno.h>
#include <string.h>
#include <unistd.h>     // close

#include "proxy_client.hpp"
#include "utils.hpp"



namespace 
{

    struct sockaddr_in resolve(const char* host, int port)
    {
        struct hostent* hp = gethostbyname(host);
        if (NULL == hp)
            throw std::runtime_error("resolve error: " + std::string(strerror(errno)));

        char** pAddr = hp->h_addr_list;
        while(*pAddr)
        {
            unsigned char *ipf = reinterpret_cast<unsigned char*>(*pAddr);
            uint32_t cur_interface_ip = 0;
            uint8_t *rimap_local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
            rimap_local_ip_ptr[0] = ipf[0];
            rimap_local_ip_ptr[1] = ipf[1];
            rimap_local_ip_ptr[2] = ipf[2];
            rimap_local_ip_ptr[3] = ipf[3];

            //std::cerr << "resolved: " << utils::int2ipv4(cur_interface_ip) << std::endl;

            ++pAddr;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = /*Address Family*/AF_INET;        // only AF_INET !
        addr.sin_port = htons(port);
        memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

        return addr;
    }

    void set_non_blocked_impl(int sd, bool opt) throw (std::exception)
    {
        int flags = fcntl(sd, F_GETFL, 0);
        int new_flags = (opt)? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        if (fcntl(sd, F_SETFL, new_flags) == -1)
            throw std::runtime_error("make nonblocked: " + std::string(strerror(errno)));
    }

    int timed_connect(const std::string &host, int port, int timeout)
    {
        struct sockaddr_in addr = resolve(host.data(), port);

        int sd = socket(/*Protocol Family*/PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd <= 0)
            throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));

        set_non_blocked_impl(sd, true);

        int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
        if (connected == -1 && errno != EINPROGRESS)
        {
            ::close(sd);
            std::cerr << "connect error: " << strerror(errno) << "\n";
            return -1;
        }

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sd, &write_fds);
        struct timeval tm;
        tm.tv_sec = timeout;
        tm.tv_usec = 0;
        int sel = select(sd + 1, /*read*/NULL, /*write*/&write_fds, /*exceptions*/NULL, &tm);

        if (sel != 1)
        {
            ::close(sd);
            std::cerr << "connect timeout\n";
            return -1;
        }

        set_non_blocked_impl(sd, false);
        return sd;
    }

    int non_blocked_connect(const std::string &host, int port)
    {
        struct sockaddr_in addr = resolve(host.data(), port);

        int sd = socket(/*Protocol Family*/PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd <= 0)
            throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));

        set_non_blocked_impl(sd, true);

        int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
        if (connected == -1 && errno != EINPROGRESS)
        {
            ::close(sd);
            std::cerr << "connect error: " << strerror(errno) << "\n";
            return -1;
        }
        return sd;
    }

    int send(int sd, const std::string &str) noexcept
    {
        size_t left = str.size();
        ssize_t sent = 0;
        //int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
        int flags = 0;

        while (left > 0)
        {
            sent = ::send(sd, str.data() + sent, str.size() - sent, flags);
            if (-1 == sent)
            {
                std::cerr << "write failed: " << strerror(errno) << "\n";
                return -1;
            }

            left -= sent;
        }
        
        return str.size();
    }

    std::string recv_bytes(int sd, size_t bytes) noexcept
    {
        char *buf = new char[bytes];
        if (buf == NULL)
        {
            std::cerr << "allocation memory error: " << strerror(errno) << "\n";
            return "";
        }
        
        size_t r = 0; 
        while (r != bytes)
        {    
            ssize_t rc = ::recv(sd, buf + r, bytes - r, 0);
            if (rc == -1 || rc == 0)
            {
                delete [] buf;
                std::cerr << "read failed: " << strerror(errno) << "\n";
                return "";
            }
            r += rc;
        }
        std::string ret(buf, buf + bytes);
        delete [] buf;
        return ret;
    }

    bool parse_http_protocol(const std::string &buf, size_t &bytes_left)
    {
        std::string::size_type hdr_end_pos = buf.find("\r\n\r\n");
        if (hdr_end_pos == std::string::npos)
            return false;

        std::string body = buf.substr(hdr_end_pos + 4, std::string::npos);

        std::string::size_type pos = buf.find("Content-Length: ");
        if (pos == std::string::npos)
        {
            bytes_left = 0;
            return true;
        }

        std::string::size_type length_pos = pos + strlen("Content-Length: ");
        size_t content_length = std::stoi(buf.substr(length_pos, buf.find("\r\n", length_pos)));

        bytes_left = content_length - body.size();
        return true;
    }

    std::string recv(int sd) noexcept
    {
        char buf[65536];
#ifdef __APPLE__
        // mac os x don't defines MSG_NOSIGNAL
        int n = ::recv(sd, buf, sizeof(buf), 0);
#else
        int n = ::recv(sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif

        if (-1 == n && errno != EAGAIN)
        {
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }
        if (0 == n)
        {
            std::cerr << "client: " + std::to_string(sd) << " disconnected\n";
            return "";
        }
        if (-1 == n)
        {
            throw std::runtime_error("client: " + std::to_string(sd) + " timeouted");
        }

        std::string ret(buf, buf + n);
        return ret;
    }


    std::string recv_http(int sd) noexcept
    {
        char buf[1024];

        size_t bytes_left = 0;
        std::string ret;

        while (true)
        {
            int n = ::recv(sd, buf, sizeof(buf), MSG_NOSIGNAL);

            if (-1 == n && errno != EAGAIN)
                throw std::runtime_error("read failed: " + std::string(strerror(errno)));
            if (0 == n)
                throw std::runtime_error("client: " + std::to_string(sd) + " disconnected");
            if (-1 == n)
                throw std::runtime_error("client: " + std::to_string(sd) + " timeouted");

            ret.append(buf, buf + n);

            if (parse_http_protocol(ret, bytes_left))
                break;
        }

        if (bytes_left > 0)
        {
            std::string left = recv_bytes(sd, bytes_left);
            ret += left;
        }

        return ret;
    }


    std::string timed_recv(int sd, int timeout) noexcept
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sd, &read_fds);
        struct timeval tm;
        tm.tv_sec = timeout;
        tm.tv_usec = 0;
        int sel = select(sd + 1, /*read*/&read_fds, /*write*/NULL, /*exceptions*/NULL, &tm);
        if (sel != 1)
        {
            std::cerr << "read timeout\n";
            return "";
        }

        return recv(sd);
    }




}   // namespace



void ProxyClient::onRead(const std::string &str)
{
    std::cerr << "read from firefox: " << str.size() << " bytes:\n";
    //std::cerr << "buf:\n" << buf << "\n";

/*
    if (sd_ssl_proxy > 0)
    {
        int rc = ::send(sd_ssl_proxy, str.data());
        if (rc <= 0)
        {
            std::cerr << "ssl send error :(\n";
        }
        else
        {
            std::cerr << "ssl send rc: " << rc << " bytes\n";
        }
        std::string resp = timed_recv(sd_ssl_proxy, 3);
        std::cerr << "ssl recv rc: " << resp.size() << "\n";


        rc = send(sd, resp);
        if (rc <= 0)
        {
            std::cerr << "back to firefox send error :(\n";
        }
        else
        {
            std::cerr << "back to firefox send rc: " << rc << " bytes\n";
        }
        continue;
    }
*/
    _req.append(str);

    if (_req.valid())
    {
        // send

        {
            _req.dump();

            if (_req._headers._method == "CONNECT")
            {
                _resp.append("HTTP/1.1 200 OK\r\n\r\n");
                _resp.dump();

                std::string host;
                std::string port;
                std::vector<std::string> host_port = utils::split(_req._headers.header("Host"), ":");

                if (host_port.size() != 2)
                {
                    host = _req._headers.header("Host");
                    port = "80";
                }
                else
                {
                    host = host_port[0];
                    port = host_port[1];
                }

                int nbsd = non_blocked_connect(host, std::stoi(port));
                Client *c = new ProxyClientToRemoteServer(nbsd, _ev, _sd, _req);
                _ev->addToEventLoop(c, event_t::EV_OUT);

                return;
            }

            if (_req._headers._method == "GET")
            {
                int nbsd = non_blocked_connect(_req._headers.header("Host"), 80);
                Client *c = new ProxyClientToRemoteServer(nbsd, _ev, _sd, _req);
                _ev->addToEventLoop(c, event_t::EV_OUT);
                return;
            }
            
            std::cerr << "UNKNOWN HTTP REQUEST!\n";
        }

        //_req.clear();

        //state = client_state_t::WANT_WRITE;
        //bool ret = set_desire_events(epfd, sock, EPOLLOUT, cs);
        //if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
    }
    else
    {
        //state = client_state_t::WANT_READ;
        //bool ret = set_desire_events(epfd, sd, EPOLLIN, cs);
        //if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
    }
}


void ProxyClient::onWrite()
{

    std::cerr << "ProxyClient onWrite\n";

}


void ProxyClientToRemoteServer::onRead(const std::string &str)
{
    std::cerr << "ProxyToRemoteServer onRead\n";

    if (_state == state::WANT_READ_FROM_TARGET)
    {

        char buf[1024];
        int n = ::recv(_sd, buf, sizeof(buf), MSG_NOSIGNAL);
        
        std::cerr << "read: " << n << " bytes\n";


        //std::string resp = recv_http(_sd);
        //std::cerr << "recv rc: " << resp.size() << "\n";

        //_resp.append(resp);
        _resp.append(std::string(buf, buf + n));


        if (_resp._chunked)
        {
            //std::cerr << "HEADERS WITH CHUNKED:\n";
            //std::cerr << _resp._headers.toString();

            size_t chunk_size = _resp._chunk_size;
            size_t cur_body_size = _resp._body.size();

            if (chunk_size > cur_body_size)
            {
                //std::string chunk = recv_bytes(sd, chunk_size - cur_body_size);
                //std::cerr << "read chunk: " << chunk.size() << " bytes\n";
                //_resp.append(chunk);
            }
            else
            {
                // скачали весь чанк типа?
                std::string chunk_to_ff = _resp._content;
                int rc = send(_sd, chunk_to_ff);
                if (rc <= 0)
                {
                    std::cerr << "back to firefox send error :(\n";
                }
                else
                {
                    std::cerr << "back to firefox send chunk: " << rc << " bytes\n";
                }

                std::cerr << "CONTINUE AFTER CHUNKED\n";
                _req.clear();
                _resp.clear();
                return;
            }

            std::string crlf = recv_bytes(_sd, 2);
            if (crlf != "\r\n")
                throw std::runtime_error("pizdec1");

            {
                std::string chunk_to_ff = _resp._content + "\r\n";
                int rc = send(_sd, chunk_to_ff);
                if (rc <= 0)
                {
                    std::cerr << "back to firefox send error :(\n";
                }
                else
                {
                    std::cerr << "back to firefox send chunk: " << rc << " bytes\n";
                }
            }

            std::string next_chunk_size;
            while (1)
            {
                std::string next_char = recv_bytes(_sd, 1);
                next_chunk_size += next_char;

                if (utils::ends_with(next_chunk_size, "\r\n"))
                {
                    next_chunk_size.pop_back();
                    next_chunk_size.pop_back();

                    size_t chunk_size = std::stoi(next_chunk_size, nullptr, 16);
                    std::cerr << "sz hex: " << next_chunk_size << ", sz dec: " << chunk_size << "\n";

                    if (chunk_size == 0)
                    {
                        std::string unused_crlf = recv_bytes(_sd, 2);
                        if (unused_crlf != "\r\n")
                            throw std::runtime_error("pizdec2");

                        std::string chunk_to_ff = "0\r\n\r\n";
                        int rc = send(_sd, chunk_to_ff);
                        if (rc <= 0)
                        {
                            std::cerr << "back to firefox send error :(\n";
                        }
                        else
                        {
                            std::cerr << "back to firefox send chunk: " << rc << " bytes\n";
                        }

                        break;
                    }

                    std::string chunk = recv_bytes(_sd, chunk_size);
                    std::cerr << "chunk recv: " << chunk.size() << " bytes\n";

                    std::string unused_crlf = recv_bytes(_sd, 2);
                    if (unused_crlf != "\r\n")
                        throw std::runtime_error("pizdec3");

                    {
                        std::string chunk_to_ff = next_chunk_size + "\r\n" + chunk + "\r\n";
                        int rc = send(_sd, chunk_to_ff);
                        if (rc <= 0)
                        {
                            std::cerr << "back to firefox send error :(\n";
                        }
                        else
                        {
                            std::cerr << "back to firefox send chunk: " << rc << " bytes\n";
                        }
                    }

                    next_chunk_size = "";
                }
                else
                {
                    continue;
                }
            }

            std::cerr << "CONTINUE AFTER CHUNKED\n";
            _req.clear();
            _resp.clear();
        }

        _resp.dump();
        return;
    }

}

void ProxyClientToRemoteServer::onWrite()
{
    std::cerr << "ProxyToRemoteServer onWrite\n";

/*
                std::string resp = cs->_resp.toString();

                write(cs->_sd, resp.data(), resp.size());
                cs->_state = client_state_t::WANT_READ;

                cs->_resp.clear();
 */


    if (_state == state::WANT_WRITE_FROM_CLI_TO_TARGET)
    {
        std::cerr << "proxy sends\n";
        _req.dump();
        int rc = send(_sd, _req.toString());
        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "send rc: " << rc << " bytes\n";
        }

        _state = state::WANT_READ_FROM_TARGET;
        _ev->changeEvents(this, event_t::EV_IN);
        return;
    }




}




