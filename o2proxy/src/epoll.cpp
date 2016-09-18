#include <sys/epoll.h>
#include <sys/socket.h>     // socket()
#include <netinet/in.h>     // htons(), INADDR_ANY
#include <netinet/tcp.h>    // SOL_TCP

#include <errno.h>
#include <assert.h>

#include <unistd.h>     // close()

#include <stdint.h>     // uint32_t
#include <string.h>

#include <iostream>
#include <set>
#include <thread>
#include <algorithm>
#include <string>

// TOTO: another place
#include <sys/socket.h> // socket(), AF_INET/PF_INET
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_aton()
#include <netdb.h>      // gethostbyname
#include <fcntl.h>      // F_GETFL, O_NONBLOCK e.t.c.



#include "epoll.hpp"
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
        char buf[256];
#ifdef __APPLE__
        // mac os x don't defines MSG_NOSIGNAL
        int n = ::recv(sd, buf, sizeof(buf), 0);
#else
        int n = ::recv(sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif

        if (-1 == n && errno != EAGAIN)
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        if (0 == n)
            throw std::runtime_error("client: " + std::to_string(sd) + " disconnected");
        if (-1 == n)
            throw std::runtime_error("client: " + std::to_string(sd) + " timeouted");

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







static bool set_desire_events(int epfd, int cli_fd, int events, Client *c)
{
    struct epoll_event ev;
    ev.data.ptr = c;
    ev.events = events;
    bool ret = (epoll_ctl(epfd, EPOLL_CTL_MOD, cli_fd, &ev) == 0);
    return ret;
}

static void accept_action(int epfd, int listener)
{
    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t cli_len = sizeof(client);

    int cli_sd = accept(listener, (struct sockaddr*)&client, &cli_len);

    //int yes = 1;
    //int keepidle = 10;  // sec
    //setsockopt(cli_sd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPCNT, &yes, sizeof(yes));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPIDLE, /*sec*/&keepidle, sizeof(keepidle));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPINTVL, &yes, sizeof(yes));

    struct epoll_event cli_ev;
    //cli_ev.data.fd = cli_sd;
    cli_ev.data.ptr = new Client(cli_sd);

    // TODO: EPOLLONESHOT
    cli_ev.events = EPOLLIN;                            // level-triggered, by default
    //cli_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;     // ET - edge-triggered

    std::cerr << "new client: " << cli_sd << ", ptr: " << (void*)cli_ev.data.ptr << "\n";
    epoll_ctl(epfd, EPOLL_CTL_ADD, cli_sd, &cli_ev);
}

void EpollEngine::eventLoop()
{
    const int max_epoll_clients = 32768;                    // 2^15
    int epfd = epoll_create(/*max cli*/max_epoll_clients);

    {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = listener();
        ev.events = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_ADD, listener(), &ev);
    }

    struct epoll_event *events = (struct epoll_event*)malloc(max_epoll_clients * sizeof(struct epoll_event));

    while (1)
    {
        std::vector<Client*> disconnected_clients;
        int epoll_ret = epoll_wait (epfd, events, max_epoll_clients, -1);

        if (epoll_ret == 0) continue;
        if (epoll_ret == -1) throw std::runtime_error(std::string("poll: ") + strerror(errno));
  
        for (int i = 0; i < epoll_ret; ++i)
        {
            if (events[i].data.fd == listener())
            {
                accept_action(epfd, listener());
                continue;
            }

            if (events[i].events & EPOLLHUP)
            {
                // e.g. previous write() was in a already closed sd
                std::cerr << "client hup\n";
                Client *cs = static_cast<Client*>(events[i].data.ptr);
                disconnected_clients.push_back(cs);
            }
            else if (events[i].events & EPOLLIN)
            {
                Client *cs = static_cast<Client*>(events[i].data.ptr);
                if (cs->state != client_state_t::WANT_READ)
                    continue;

                int sock = cs->sd;

                char buf[1024];
                int r = read(sock, buf, sizeof(buf));
                buf[r] = '\0';

                if (r < 0)
                {
                    std::cerr << "some read error!\n";
                    disconnected_clients.push_back(cs);
                    continue;
                }

                if (r > 0)
                {
                    std::cerr << "read from firefox: " << r << " bytes:\n";
                    //std::cerr << "buf:\n" << buf << "\n";

                    cs->_req.append(std::string(buf, buf + r));

                    if (cs->_req.valid())
                    {
                        // send

                        {
                            cs->_req.dump();
                            //std::cerr << cs->_req._headers.toString();

                            int sd = timed_connect(cs->_req._headers.header("Host"), 80, 10);
                            int rc = send(sd, cs->_req.toString());
                            if (rc <= 0)
                            {
                                std::cerr << "send error :(\n";
                            }
                            else
                            {
                                std::cerr << "send rc: " << rc << " bytes\n";
                            }
                            std::string resp = recv_http(sd);
                            std::cerr << "recv rc: " << resp.size() << "\n";

                            cs->_resp.append(resp);
                            cs->_resp.dump();
                        }

                        cs->state = client_state_t::WANT_WRITE;
                        bool ret = set_desire_events(epfd, sock, EPOLLOUT, cs);
                        if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
                    }
                    else
                    {
                        cs->state = client_state_t::WANT_READ;
                        bool ret = set_desire_events(epfd, cs->sd, EPOLLIN, cs);
                        if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
                    }
                }
                else if (r == 0)
                {
                    std::cerr << "client disconnected: " << sock << "\n";
                    disconnected_clients.push_back(cs);
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                Client *cs = static_cast<Client*>(events[i].data.ptr);
                if (cs->state != client_state_t::WANT_WRITE)
                    continue;

                std::string resp = cs->_resp.toString();

                write(cs->sd, resp.data(), resp.size());
                cs->state = client_state_t::WANT_READ;

                bool ret = set_desire_events(epfd, cs->sd, EPOLLIN, cs);
                if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
            }
            else
            {
                Client *cs = (Client*)events[i].data.ptr;
                if (events[i].events & EPOLLERR)
                    std::cerr<<"WARNING> events = EPOLLERR. [SD = " << cs->sd <<"]\n";
                else
                    std::cerr<<"WARNING> event = UNKNOWN_EVENT: "<< events[i].events <<" [SD = " << cs->sd << "]\n";
            }
        }       //for (int i = 0; i < epoll_ret; ++i)

        // remove disconnected clients
        if (disconnected_clients.size())
        {
            std::cerr << "GC start. disconnect: " << disconnected_clients.size() << " clients" << std::endl;
            for (size_t i = 0; i < disconnected_clients.size(); ++i)
            {
                int sd = disconnected_clients[i]->sd;
                std::cerr << "close: " << sd << ", delete: " << (void*)disconnected_clients[i] << std::endl;
                close(sd);
                delete disconnected_clients[i];
            }
        }
    }
}

void EpollEngine::run()
{
    std::cerr << "epoll server starts" << std::endl;
    eventLoop();
}
