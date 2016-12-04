#include "config.hpp"

Config::Config(const std::string &path)
{
    init();
}

void Config::init()
{
    _port = 7788;
    _engine = engine_t::EPOLL;
}
