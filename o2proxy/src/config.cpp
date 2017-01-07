#include <pwd.h>
#include <unistd.h>

#include <vector>
#include <fstream>
#include <iomanip>

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

Config* Config::impl()
{
    if (_self == NULL)
    {
        _self = new Config();
    }
    return _self;
}

void Config::destroy()
{
    delete _self;
    _self = nullptr;
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
{
    std::vector<std::string> lines = utils::split(config, "\n");
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::vector<std::string> pair = utils::split(lines[i], ":", 2);
        if (pair.size() < 2)
        {
            continue;
        }

        std::string key = utils::trimmed(pair[0]);
        std::string value = utils::trimmed(pair[1]);

        std::pair<SettingItem &, bool> item = m_Storage.find_option_by_long_key(key);
        if (!item.second)
        {
            logw("unused key in config: ", key);
            continue;
        }

        parseFromConfig(item.first.value(), item.first.value().type(), value);
    }
}

void Config::parseFromConfig(AnyItem &item, AnyItem::type_t type, const std::string &text)
{
    if (type == AnyItem::INT)
    {
        int v = std::stoi(text);
        item.store(v);
    }
    else if (type == AnyItem::DOUBLE)
    {
        double v = std::stod(text);
        item.store(v);
    }
    else if (type == AnyItem::STRING)
    {
        item.store(text);
    }
    else if (type == AnyItem::ADDRESS)
    {
        std::vector<std::string> tmp = utils::split(text, ":");
        if (tmp.size() != 2)
        {
            throw std::runtime_error("parse config: value is not address - " + text);
        }
        any::address_t address;
        address.host = tmp[0];
        address.port = std::stoi(tmp[1]);
        item.store(address);
    }
    else if (type == AnyItem::FILE)
    {
        std::ifstream ifs(text.c_str());
        if (!ifs.good())
        {
            throw std::runtime_error("cannot open file \"" + text + "\"");
        }

        std::string content((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());

        any::file_t file;
        file.name = text;
        file.content = content;
        item.store(file);
    }
    else if (type == AnyItem::VECTOR)
    {
        std::vector<std::string> tmp = utils::split(text, ",");
        for (size_t i = 0; i < tmp.size(); ++i)
        {
            tmp[i] = utils::trimmed(tmp[i]);
            AnyItem any;
            parseFromConfig(any, item.vectorType(), tmp[i]);
            item.pushBack(any);
        }
    }
}

void Config::dump() const
{
    std::stringbuf str;
    std::ostream os(&str);

    os << "Config dump: " << std::endl;
    for (const auto &i : m_Storage.items())
    {
        os << "\t" << std::setw(16) << std::left << i.second.lkey();
        os << ": " << i.second.value() << std::endl;
    }
    logi(str.str());
}

std::string Config::usage() const
{
    std::stringstream ss;
    ss << "Config options description: " << std::endl;

    for (const auto &i : m_Storage.items())
    {
        const SettingItem &item = i.second;
        std::string param = item.lkey();

        ss << "\t" << std::setw(24) << std::left << param
           << ": " << item.desc() << std::endl;
    }

    return ss.str();
}
