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

#include "epoll.hpp"
#include "utils.hpp"
#include "proxy_client.hpp"


static void accept_action(int epfd, int listener, Engine *ev)
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
    memset(&cli_ev, 0, sizeof(struct epoll_event));

    // NB: data - is an UNION !!!
    cli_ev.data.ptr = new ProxyClient(cli_sd, ev);

    // TODO: EPOLLONESHOT
    cli_ev.events = EPOLLIN;                            // level-triggered, by default
    //cli_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;     // ET - edge-triggered

    if (0 != epoll_ctl(epfd, EPOLL_CTL_ADD, cli_sd, &cli_ev))
    {
        std::cerr << "error add to epoll!\n";
    }
    else
    {
        std::cerr << "new client: " << cli_sd << ", ptr: " << (void*)cli_ev.data.ptr << "\n";
    }

}

void EpollEngine::changeEvents(Client *c, engine::event_t events)
{
    struct epoll_event ev;
    ev.data.ptr = c;

    if (events == engine::event_t::EV_READ)
    {
        ev.events = EPOLLIN;
        c->_state = client_state_t::WANT_READ;
    }
    else if (events == engine::event_t::EV_WRITE)
    {
        ev.events = EPOLLOUT;
        c->_state = client_state_t::WANT_WRITE;
    }

    bool ret = (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, c->_sd, &ev) == 0);

    if (!ret)
    {
        std::cerr << "epol_ctl error!\n";
    }

    return;
}

void EpollEngine::addToEventLoop(Client *c, engine::event_t events)
{
    struct epoll_event cli_ev;
    memset(&cli_ev, 0, sizeof(struct epoll_event));
    cli_ev.data.ptr = c;

    if (events == engine::event_t::EV_READ)
    {
        cli_ev.events = EPOLLIN;
        c->_state = client_state_t::WANT_READ;
    }
    else if (events == engine::event_t::EV_WRITE)
    {
        cli_ev.events = EPOLLOUT;
        c->_state = client_state_t::WANT_WRITE;
    }

    if (0 != epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, c->_sd, &cli_ev))
    {
        std::cerr << "error add to epoll!\n";
    }
    else
    {
        std::cerr << "added to event loop: " << c->_sd << ", ptr: " << (void*)cli_ev.data.ptr << "\n";
    }
}

void EpollEngine::eventLoop()
{
    const int max_epoll_clients = 32768;                    // 2^15
    _epoll_fd = epoll_create(/*max cli*/max_epoll_clients);

    {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = listener();
        ev.events = EPOLLIN;
        epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener(), &ev);
    }

    struct epoll_event *events = (struct epoll_event*)malloc(max_epoll_clients * sizeof(struct epoll_event));

    while (1)
    {
        std::vector<Client*> disconnected_clients;
        int epoll_ret = epoll_wait (_epoll_fd, events, max_epoll_clients, -1);

        //std::cerr << "epoll_ret: " << epoll_ret << "\n";

        if (epoll_ret == 0) continue;
        if (epoll_ret == -1) throw std::runtime_error(std::string("poll: ") + strerror(errno));
  
        for (int i = 0; i < epoll_ret; ++i)
        {
            //std::cerr << "test: " << events[i].data.fd  << ", " << (void*)events[i].data.ptr << "\n";
            //sleep(1);

            if (events[i].data.fd == listener())
            {
                accept_action(_epoll_fd, listener(), this);
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
                Client *c = static_cast<Client*>(events[i].data.ptr);
                if (c->_state != client_state_t::WANT_READ)
                    continue;

                char buf[65536];
                int r = read(c->_sd, buf, sizeof(buf));
                buf[r] = '\0';

                if (r < 0)
                {
                    std::cerr << "read error! " << strerror(errno) << std::endl;
                    disconnected_clients.push_back(c);
                    continue;
                }
                else if (r == 0)
                {
                    std::cerr << "client disconnected: " << c->_sd << "\n";
                    disconnected_clients.push_back(c);
                }

                if (r > 0)
                {
                    c->onRead(std::string(buf, buf + r));
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                Client *c = static_cast<Client*>(events[i].data.ptr);
                if (c->_state != client_state_t::WANT_WRITE)
                {
                    continue;
                }

                c->onWrite();

                //bool ret = set_desire_events(_epoll_fd, cs->_sd, EPOLLIN, cs);
                //if (!ret) throw std::runtime_error(std::string("epoll_ctl error: ") + strerror(errno));
            }
            else
            {
                Client *cs = (Client*)events[i].data.ptr;
                if (events[i].events & EPOLLERR)
                    std::cerr<<"WARNING> events = EPOLLERR. [SD = " << cs->_sd <<"]\n";
                else
                    std::cerr<<"WARNING> event = UNKNOWN_EVENT: "<< events[i].events <<" [SD = " << cs->_sd << "]\n";
            }
        }       //for (int i = 0; i < epoll_ret; ++i)

        // remove disconnected clients
        if (disconnected_clients.size())
        {
            std::cerr << "GC start. disconnect: " << disconnected_clients.size() << " clients" << std::endl;
            for (size_t i = 0; i < disconnected_clients.size(); ++i)
            {
                int sd = disconnected_clients[i]->_sd;
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
