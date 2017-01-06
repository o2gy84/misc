#include "server.hpp"
#include "poll.hpp"
#include "poll_async.hpp"
#include "epoll.hpp"
#include "select.hpp"
#include "logger.hpp"
#include "config.hpp"

namespace 
{
    std::unique_ptr<Engine> get_engine(config::engine_t type, int port)
    {
        std::unique_ptr<Engine> ret;
        switch (type)
        {
            case config::engine_t::SELECT:
            {
                ret.reset(new SelectEngine(port));
                break;
            }
            case config::engine_t::POLL:
            {
                //if (async) ret.reset(new AsyncPollEngine(port));
                if (1)      ret.reset(new AsyncPollEngine(port));
                else        ret.reset(new PollEngine(port));
                break;
            }
            case config::engine_t::EPOLL:
            {
                //if (async) ret.reset(new EpollEngine(port));
                if (1)     ret.reset(new EpollEngine(port));
                else       ret.reset(new EpollEngine(port)); 
                break;
            }
            default: throw std::logic_error("unknown engine type");
        }

        logi("use engine: ", engine2string(type));
        return ret;
    }
}

Server::Server(const Options &opt)
{
    int port = Config::impl()->get<int>("port");

    int opt_port = opt.get<int>("port");
    if (opt_port)
    {
        // opt overrides
        port = opt_port;
    }

    config::engine_t engine_type = config::engine_t::UNKNOWN;
    engine_type = config::string2engine(Config::impl()->get<std::string>("engine"));
    m_Engine = get_engine(engine_type, port);
}

void Server::run()
{
    m_Engine->run();
}
