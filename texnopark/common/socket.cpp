#include <iostream>
#include <stdexcept>

#include <string.h>
#include <sys/socket.h> // socket(), AF_INET/PF_INET
#include <netinet/in.h> // struct sockaddr_in
//#include <arpa/inet.h>  // inet_aton()
#include <netdb.h>      // gethostbyname
#include <fcntl.h>

#include "socket.hpp"

namespace 
{

std::string int2ipv4(uint32_t ip)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip&0xFF, (ip&0xFF00) >> 8, (ip&0xFF0000) >> 16, (ip&0xFF000000) >> 24);
    return buf;
}

int connect_impl(const char* host, int port)
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
        std::cerr << "resolved: " << int2ipv4(cur_interface_ip) << std::endl;
        ++pAddr;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = /*Address Family*/AF_INET;        // only AF_INET !
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    int sd = socket(/*Protocol Family*/PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd <= 0)
        throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));

    int connected = connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    if (connected == -1)
    {
        throw std::runtime_error("connect error: " + std::string(strerror(errno)));
        close(sd);
    }

    return sd;
}
}   // namespace

void Socket::setNonBlocked(bool opt) throw (std::exception)
{
    int flags = fcntl(m_Sd, F_GETFL, 0);
    int new_flags = (opt)? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(m_Sd, F_SETFL, new_flags) == -1)
        throw std::runtime_error("make nonblocked: " + std::string(strerror(errno)));
}

void Socket::setRcvTimeout(int sec, int microsec) throw (std::exception)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = microsec;

    if (setsockopt(m_Sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
        throw std::runtime_error("set rcvtimeout: " + std::string(strerror(errno)));
}

void Socket::connect(const std::string &host, int port) throw (std::exception)
{
    m_Sd = connect_impl(host.data(), port);
}

void Socket::send(const std::string &str) throw (std::exception)
{
    size_t left = str.size();
    ssize_t sent = 0;
    //int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
    int flags = 0;

    std::cerr << "send:\n" << str << std::endl;

    while (left > 0)
    {
        sent = ::send(m_Sd, str.data() + sent, str.size() - sent, flags);
        if (-1 == sent)
            throw std::runtime_error("write failed: " + std::string(strerror(errno)));

        left -= sent;
    }
}

std::string Socket::recv(size_t bytes) throw (std::exception)
{
    char *buf = new char[bytes];
    size_t r = 0; 
    while (r != bytes)
    {    
        ssize_t rc = ::recv(m_Sd, buf + r, bytes - r, 0);
        std::cerr << "recv_ex: " << rc << " bytes\n";

        if (rc == -1 || rc == 0)
        {
            delete [] buf;
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }
        r += rc;
    }
    std::string ret(buf, buf + bytes);
    delete [] buf;
    return ret;
}

namespace
{

bool parse_protocol(const std::string &buf, size_t &bytes_left)
{
    std::string::size_type hdr_end_pos = buf.find("\r\n\r\n");
    if (hdr_end_pos == std::string::npos)
        return false;

    std::string body = buf.substr(hdr_end_pos + 4, std::string::npos);

    std::string::size_type pos = buf.find("Content-Length: ");
    if (pos == std::string::npos)
        throw std::runtime_error("http broken");

    std::string::size_type length_pos = pos + strlen("Content-Length: ");
    size_t content_length = std::stoi(buf.substr(length_pos, buf.find("\r\n", length_pos)));

    bytes_left = content_length - body.size();
    return true;
}

}

std::string Socket::recv() throw (std::exception)
{
    std::string ret;
    char buf[128];
    //int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
    int flags = 0;

    while (true)
    {
        int n = ::recv(m_Sd, buf, sizeof(buf), flags);
        
        std::cerr << "recv: " << n << " bytes\n";

        if (-1 == n && errno != EAGAIN)
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));

        if (0 == n || -1 == n)
            break;
        
        size_t bytes_left = 0;
        ret.append(buf, n);
        if (parse_protocol(ret, bytes_left))
            return ret + recv(bytes_left);
    }

    return ret;
}


