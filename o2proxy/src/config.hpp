#pragma once

#include <string>

#include "settings_storage.hpp"

namespace config
{

enum class engine_t: uint8_t 
{
    UNKNOWN,
    SELECT,
    POLL,
    EPOLL
};

engine_t string2engine(const std::string &str);
std::string engine2string(engine_t engine);

}

/*
    Programm config class.
*/
class Config
{
public:
    static Config* impl();
    void load(const std::string &path);
    void dump() const;
    void destroy();
    std::string usage() const;

public:
    template <typename T>
    void add(const std::string &lk, const std::string &desc, T&& default_value)
    {
        if (lk.empty())
        {
            throw std::runtime_error("key is empty: " + lk);
        }
        m_Storage.addValueByKey(lk, "", desc, default_value);
    }

    template <typename T>
    T get(const std::string &key) const
    {
        std::pair<SettingItem &, bool> ret = m_Storage.find_option_by_long_key(key);
        if (!ret.second)
        {
            throw std::runtime_error("no such options: " + key);
        }
        return ret.first.value().get<T>();
    }

private:
    Config() {};
    Config(const Config &);
    const Config& operator=(const Config &);

private:
    void read(const std::string &path);
    void parse(const std::string &config);
    void parseFromConfig(AnyItem &item, AnyItem::type_t type, const std::string &text);

private:
    static Config *_self;

    SettingsStorage m_Storage;
};
