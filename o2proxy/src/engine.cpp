#include <stdint.h>     // uint32_t
#include <sys/socket.h> // socket()
#include <netinet/in.h> // htons(), INADDR_ANY
#include <string.h>     // memset()
#include <unistd.h>     // close()
#include <errno.h>
#include <netdb.h>      // gethostbyname
#include <signal.h>     // signal

#include <stdexcept>
#include <string>

#include <iostream>
#include <algorithm>

#include "utils.hpp"
#include "engine.hpp"
#include "logger.hpp"
#include "config.hpp"

static const uint32_t kListenQueueSize = 1024;

std::atomic<bool> Engine::g_Stop(false);

void sig_handler(int signum)
{
    Engine::g_Stop = true;
    Config::impl()->destroy();
}

namespace
{
    void insert_resolved_ips(const std::string &host, std::vector<uint32_t> &ips)
    {
        if (host.empty()) return;

        struct hostent* hp = gethostbyname(host.c_str());
        if (NULL == hp)
        {
            std::string e = std::string("[host: ") + host + ", e: " + std::string(strerror(errno)) + "]";
            throw std::runtime_error("resolve error " + e);
        }

        char** pAddr = hp->h_addr_list;
        while (*pAddr)
        {
            unsigned char *ipf = reinterpret_cast<unsigned char*>(*pAddr);
            uint32_t cur_interface_ip = 0;
            uint8_t *local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
            local_ip_ptr[0] = ipf[0];
            local_ip_ptr[1] = ipf[1];
            local_ip_ptr[2] = ipf[2];
            local_ip_ptr[3] = ipf[3];

            logi("localhost resolved: {0}, as_uint: {1}", utils::int2ipv4(cur_interface_ip), cur_interface_ip);

            ++pAddr;
            ips.push_back(cur_interface_ip);
        }

    }
}

Engine::Engine(int port)
{
    signal(SIGINT, sig_handler);

    insert_resolved_ips("localhost", m_LocalIPs);
    insert_resolved_ips(Config::impl()->get<std::string>("local_address"), m_LocalIPs);

    m_Listener = listenSocket(port, kListenQueueSize);
    if (m_Listener <= 0)
    {
        std::string what = std::string("error listen socket [port: ") + std::to_string(port);
        what += std::string(", e: ") + strerror(errno) + "]";
        throw std::runtime_error(what);
    }
}

Engine::~Engine()
{
    close(m_Listener);
}


bool Engine::isMyHost(const std::string &host) const
{
    struct hostent* hp = gethostbyname(host.data());
    if (NULL == hp)
    {
        loge("engine: [{0}] resolve error: {1}", host, std::string(strerror(errno)));
        return false;
    }

    char** pAddr = hp->h_addr_list;
    while (*pAddr)
    {
        unsigned char *ipf = reinterpret_cast<unsigned char*>(*pAddr);
        uint32_t cur_interface_ip = 0;
        uint8_t *local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
        local_ip_ptr[0] = ipf[0];
        local_ip_ptr[1] = ipf[1];
        local_ip_ptr[2] = ipf[2];
        local_ip_ptr[3] = ipf[3];

        auto it = std::find(m_LocalIPs.begin(), m_LocalIPs.end(), cur_interface_ip);
        if (it != m_LocalIPs.end())
            return true;

        ++pAddr;
    }

    return false;
}


int Engine::listenSocket(uint32_t port, uint32_t listen_queue_size)
{
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd <= 0)
        return -1;

    int yes = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        return -1;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    if (1)
    {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        // TODO: !!!
        struct hostent *hp = gethostbyname("192.168.70.129");
        serv_addr.sin_addr.s_addr = *((unsigned long *)hp->h_addr);
    }
    serv_addr.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        return -1;
    }

    if (listen(sd, listen_queue_size) != 0)
    {
        return -1;
    }

    logi("listen to: {0}, sd: {1}", port, sd);
    return sd;
}

