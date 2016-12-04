#include <pwd.h>
#include <unistd.h>

#include <vector>
#include <fstream>

#include "utils.hpp"
#include "logger.hpp"
#include "config.hpp"

namespace config
{

std::string get_default_path()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);

    if (!pw)
    {
        return "";
    }
    std::string config = std::string(pw->pw_dir) + "/.o2proxy.conf";
    return config;
}

engine_t string2engine(const std::string &str)
{
    if (str == "select") return engine_t::SELECT;
    if (str == "poll") return engine_t::POLL;
    if (str == "epoll") return engine_t::EPOLL;
    return engine_t::UNKNOWN;
}

std::string engine2string(engine_t engine)
{
    if (engine == engine_t::SELECT) return "select";
    if (engine == engine_t::POLL)   return "poll";
    if (engine == engine_t::EPOLL)  return "epoll";
    return "unknown";
}


}

Config* Config::_self = nullptr;

Config* Config::get()
{
    if (_self == NULL)
    {
        _self = new Config();
        _self->init();
    }
    return _self;
}

void Config::init()
{
    _port = 7788;
    _engine = config::engine_t::EPOLL;
}

void Config::load(const std::string &path)
{
    std::string real_path = path;
    if (real_path.empty())
    {
        real_path = config::get_default_path();
    }

    read(real_path);
}

void Config::read(const std::string &path)
{
    if (path.empty()) return;

    logi("read config: ", path);

    std::ifstream ifs(path.c_str());
    if (!ifs.good())
    {
        logw("cannot open config file \"" + path + "\"");
        return;
    }

    std::string config((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());

    parse(config);
}

void Config::parse(const std::string &config)
// TODO: make it compatible with libconfig: http://www.hyperrealm.com/libconfig/libconfig.html
{
    std::vector<std::string> lines = utils::split(config, "\n");
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::vector<std::string> pair = utils::split(lines[i], ":");
        if (pair.size() < 2)
        {
            continue;
        }

        std::string key = utils::trimmed(pair[0]);
        std::string value = utils::trimmed(pair[1]);

        if (key == "local_address")
        {
            _local_address = value;
        }
        else if (key == "port")
        {
            _port = std::stoi(value);
        }
        else if (key == "engine")
        {
            _engine = config::string2engine(value);
        }
    }
}
