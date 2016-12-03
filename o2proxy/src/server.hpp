#include <memory>

#include "engine.hpp"
#include "options.hpp"
#include "config.hpp"


class Server
{
public:
    Server(const Options &opt, const Config &conf);
    void run();

private:
    std::unique_ptr<Engine> m_Engine;
};
