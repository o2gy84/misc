#include "config.hpp"

Config::Config(const std::string &path)
{
    engine = engine_t::EPOLL;
}

