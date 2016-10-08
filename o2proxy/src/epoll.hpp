#ifndef _EPOLL_HPP
#define _EPOLL_HPP

#include "engine.hpp"

class EpollEngine: public Engine
{
public:
    explicit EpollEngine(int port): Engine(port) {}
    virtual void run() override;
    virtual void addToEventLoop(Client *c, engine::event_t events) override;
    virtual void changeEvents(Client *c, engine::event_t events) override;

private:
    void eventLoop();

private:
    int _epoll_fd;

};

#endif
