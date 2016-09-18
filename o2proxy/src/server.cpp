#include "server.hpp"
#include "poll.hpp"
#include "poll_async.hpp"
#include "epoll.hpp"
#include "select.hpp"

namespace 
{
    std::unique_ptr<Engine> get_engine(engine_t type, int port)
    {
        std::unique_ptr<Engine> ret;
        switch (type)
        {
            case engine_t::SELECT: ret.reset (new SelectEngine(port)); break;
            case engine_t::POLL:
            {
                //if (async) ret.reset (new AsyncPollEngine(port));
                if (1)      ret.reset (new AsyncPollEngine(port));
                else        ret.reset (new PollEngine(port));
                break;
            }
            case engine_t::EPOLL:
            {
                //if (async) ret.reset (new EpollEngine(port));
                if (1)     ret.reset (new EpollEngine(port));
                else       ret.reset (new EpollEngine(port)); 
                break;
            }
            case engine_t::UNKNOWN: throw std::logic_error("unknown engine type");
        }

        return ret;
    }
}

Server::Server(const Options &opt, const Config &conf)
{
    int port = conf.port;
    if (opt._port)
    {
        // opt overrides
        port = opt._port;
    }

    m_Engine = get_engine(conf.engine, port);
}

void Server::run()
{
    m_Engine->run();
}

