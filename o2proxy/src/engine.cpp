#include <stdint.h>     // uint32_t
#include <sys/socket.h> // socket()
#include <netinet/in.h> // htons(), INADDR_ANY
#include <string.h>     // memset()
#include <unistd.h>     // close()
#include <errno.h>
#include <netdb.h>      // gethostbyname

#include <stdexcept>
#include <string>

#include <iostream>
#include <algorithm>

#include "utils.hpp"
#include "engine.hpp"
#include "logger.hpp"

static const uint32_t kListenQueueSize = 1024;

Engine::Engine(int port)
{
    struct hostent* hp = gethostbyname("localhost");
    if (NULL == hp)
        throw std::runtime_error("resolve localhost error: " + std::string(strerror(errno)));

    char** pAddr = hp->h_addr_list;
    while (*pAddr)
    {
        unsigned char *ipf = reinterpret_cast<unsigned char*>(*pAddr);
        uint32_t cur_interface_ip = 0;
        uint8_t *rimap_local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
        rimap_local_ip_ptr[0] = ipf[0];
        rimap_local_ip_ptr[1] = ipf[1];
        rimap_local_ip_ptr[2] = ipf[2];
        rimap_local_ip_ptr[3] = ipf[3];

        logi("localhost resolved: {0}, as_uint: {1}", utils::int2ipv4(cur_interface_ip), cur_interface_ip);

        ++pAddr;
        m_LocalIPs.push_back(cur_interface_ip);
    }

    m_LocalIPs.push_back(1949031455);   // 31.220.43.116

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
        uint8_t *rimap_local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
        rimap_local_ip_ptr[0] = ipf[0];
        rimap_local_ip_ptr[1] = ipf[1];
        rimap_local_ip_ptr[2] = ipf[2];
        rimap_local_ip_ptr[3] = ipf[3];

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

    listen(sd, listen_queue_size);

    logi("listen to: ", port);
    return sd;
}

