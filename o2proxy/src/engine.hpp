#ifndef _ENGINE_HPP
#define _ENGINE_HPP

#include <vector>
#include <functional>
#include <memory>
#include <atomic>

#include "client.hpp"

namespace engine
{
    enum class op_error_t: uint8_t
    {
        OP_EOF,
        OP_HUP
    };

    // Абстрактные event'ы, за которыми должен уметь следить Engine
    enum class event_t: uint8_t
    {
        EV_NONE,
        EV_READ,
        EV_WRITE
    };

    struct OperationState
    {
        size_t transferred;
        op_error_t error;
    };
}


class Engine
{
public:
    static std::atomic<bool> g_Stop;

public:
    explicit Engine(int port);
    ~Engine();

    virtual void run() =0;
    virtual bool addToEventLoop(Client *c, engine::event_t events) =0;
    virtual void changeEvents(Client *c, engine::event_t events) =0;

    bool isMyHost(const std::string &host) const;

protected:
    int listener() const { return m_Listener; }

private:
    int listenSocket(uint32_t port, uint32_t listen_queue_size);

private:
    int m_Listener;

    std::vector<uint32_t> m_LocalIPs;
};


#endif  // _ENGINE_HPP
