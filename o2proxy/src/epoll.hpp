#pragma once 

#include "engine.hpp"

class EpollEngine: public Engine
{
public:
    explicit EpollEngine(int port): Engine(port) {}
    virtual void run() override;
    virtual bool addToEventLoop(Client *c, engine::event_t events) override;
    virtual void changeEvents(Client *c, engine::event_t events) override;

private:
    void eventLoop();

private:
    int _epoll_fd;

};
