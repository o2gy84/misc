#include <sys/epoll.h>
#include <sys/socket.h>     // socket()
#include <netinet/in.h>     // htons(), INADDR_ANY
#include <netinet/tcp.h>    // SOL_TCP
#include <signal.h>

#include <errno.h>
#include <assert.h>

#include <unistd.h>     // close()

#include <stdint.h>     // uint32_t
#include <string.h>

#include <atomic>
#include <iostream>
#include <set>
#include <thread>
#include <algorithm>
#include <string>

#include "epoll.hpp"
#include "utils.hpp"
#include "proxy_client.hpp"
#include "logger.hpp"


namespace
{

std::atomic<bool> g_Stop(false);

void sig_handler(int signum)
{
    g_Stop = true;
}

}


static void accept_action(int epfd, int listener, Engine *ev)
{
    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t cli_len = sizeof(client);

    int cli_sd = accept(listener, (struct sockaddr*)&client, &cli_len);

    // TODO: set some options?
    //int yes = 1;
    //int keepidle = 10;  // sec
    //setsockopt(cli_sd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPCNT, &yes, sizeof(yes));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPIDLE, /*sec*/&keepidle, sizeof(keepidle));
    //setsockopt(cli_sd, SOL_TCP, TCP_KEEPINTVL, &yes, sizeof(yes));

    struct epoll_event cli_ev;
    memset(&cli_ev, 0, sizeof(struct epoll_event));

    ProxyClient *pc = new ProxyClient(cli_sd, ev);
    pc->setClientIP(client.sin_addr.s_addr);

    // NB: data - is an UNION
    cli_ev.data.ptr = pc;

    // use level-triggered (LT), by default.
    // edge-triggered (ET) mode doesn't good thing in this case, because 
    // when browser works via ssl, we don't know about how much data
    // we need to read, so we just read all available data on each EPOLLIN event.
    cli_ev.events = EPOLLIN;

    if (0 != epoll_ctl(epfd, EPOLL_CTL_ADD, cli_sd, &cli_ev))
    {
        loge("error add sd to epoll: ", strerror(errno));
    }
    else
    {
        logd2("new client: {0}, ptr: {1}", cli_sd, (void*)cli_ev.data.ptr);
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
    else if (events == engine::event_t::EV_NONE)
    {
        logd4("no waiting for events [sd: {0}, ptr: {1}]", c->_sd, (void*)c);
        ev.events = 0;
    }

    bool ret = (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, c->_sd, &ev) == 0);

    if (!ret)
    {
        loge("epol_ctl error on sd: ", c->_sd);
    }

    return;
}

bool EpollEngine::addToEventLoop(Client *c, engine::event_t events)
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
        loge("add event on sd error [sd: {0}, e: {1}]", c->_sd, strerror(errno));
        return false;
    }

    logd3("added to event loop [sd: {0}, ptr: {1}]", c->_sd, (void*)cli_ev.data.ptr);
    return true;
}

void EpollEngine::eventLoop()
{
    logi("start even loop");

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

    while (!g_Stop)
    {
        std::vector<Client*> disconnected_clients;
        int epoll_ret = epoll_wait(_epoll_fd, events, max_epoll_clients, -1);

        //logd5("epoll_ret: ", epoll_ret);

        if (epoll_ret == 0) continue;
        if (epoll_ret == -1)
        {
            if (g_Stop)
            {
                break;
            }
            throw std::runtime_error(std::string("epoll: ") + strerror(errno));
        }
  
        for (int i = 0; i < epoll_ret; ++i)
        {
            //logd5("test [sd: {0}, ptr: {1}], events[i].data.fd, (void*)events[i].data.ptr);
            //sleep(1);

            if (events[i].data.fd == listener())
            {
                accept_action(_epoll_fd, listener(), this);
                continue;
            }

            if (events[i].events & EPOLLHUP)
            {
                // e.g. previous write() was in a already closed sd
                Client *c = static_cast<Client*>(events[i].data.ptr);
                logw("hup on sd: ", c->_sd);

                disconnected_clients.push_back(c);
            }
            else if (events[i].events & EPOLLIN)
            {
                Client *c = static_cast<Client*>(events[i].data.ptr);
                if (c->_state != client_state_t::WANT_READ)
                    continue;

                char buf[65536 * 2];
                memset(buf, 0, sizeof(buf));

                int r = ::read(c->_sd, buf, sizeof(buf) - 1);
                buf[r] = '\0';

                if (r < 0 && errno != EAGAIN)
                {
                    loge("read error [sd: {0}, e: {1}]", c->_sd, strerror(errno));
                    disconnected_clients.push_back(c);
                }
                if (r < 0 && errno == EAGAIN)
                {
                    loge("egain, strange case [sd: {0}, e: {1}]", c->_sd, strerror(errno));
                    disconnected_clients.push_back(c);
                }
                else if (r == 0)
                {
                    logd3("read EOF, disconnect on sd: ", c->_sd);
                    disconnected_clients.push_back(c);
                }
                else if (r > 0)
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
            }
            else
            {
                Client *cs = (Client*)events[i].data.ptr;
                if (events[i].events & EPOLLERR)
                    loge("event epollerr on sd: ", cs->_sd);
                else
                    loge("event unknown [sd: {0}, events: {1}]", cs->_sd, static_cast<unsigned>(events[i].events));
            }
        }

        // remove disconnected clients
        if (disconnected_clients.size())
        {
            logd2("GC start. disconnected clients: ", disconnected_clients.size());
            for (size_t i = 0; i < disconnected_clients.size(); ++i)
            {
                // auto delete from epoll
                int sd = disconnected_clients[i]->_sd;
                logd3("close [sd: {0}, ptr: {1}]", sd, (void*)disconnected_clients[i]);
                close(sd);

                disconnected_clients[i]->onDead();
                delete disconnected_clients[i];
                disconnected_clients[i] = NULL;
            }
        }
    }

    logi("server stopped");
    free(events);
    close(_epoll_fd);

    // TODO: need delete all the clients
}

void EpollEngine::run()
{
    signal(SIGINT, sig_handler);
    eventLoop();
}
