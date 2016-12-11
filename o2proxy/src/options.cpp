#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "options.hpp"
#include "utils.hpp"
#include "logger.hpp"


template <>
void Options::add<bool>(const std::string &lk, const std::string &k, const std::string &desc, bool default_value)
{
    m_Items[k] = SettingItem(lk, k, desc, default_value);
    m_LongKeys.insert(lk);
}

template <>
void Options::add<int>(const std::string &lk, const std::string &k, const std::string &desc, int default_value)
{
    m_Items[k] = SettingItem(lk, k, desc, default_value);
    m_LongKeys.insert(lk);
}

template <>
void Options::add<const char*>(const std::string &lk, const std::string &k, const std::string &desc, const char *default_value)
{
    m_Items[k] = SettingItem(lk, k, desc, std::string(default_value));
    m_LongKeys.insert(lk);
}

template <>
void Options::add<std::vector<std::string>>(const std::string &k, const std::string &lk, const std::string &desc,
    std::vector<std::string> default_value)
{
    //std::cerr << "specialization of strings vector...\n";
}

std::map<std::string, SettingItem>::const_iterator Options::iterator(const std::string &k) const throw (std::exception)
{
    if (k.empty())
    {
        throw std::runtime_error("request for empty option");
    }

    auto it = m_Items.find(k.substr(0, 1));
    if (it == m_Items.end())
    {
        throw std::runtime_error("no such option: " + k);
    }

    return it;
}

template <>
bool Options::get<bool>(const std::string &k) const
{
    auto it = iterator(k);
    return it->second.value().getBool();
}

template <>
int Options::get<int>(const std::string &k) const
{
    auto it = iterator(k);
    return it->second.value().getInt();
}

template <>
std::string Options::get<std::string>(const std::string &k) const
{
    auto it = iterator(k);
    return it->second.value().getString();
}

std::string Options::usage(const std::string &progname) const
{
    std::stringstream ss;
    ss << "Usage: " << progname;

    for (const auto &i : m_Items)
    {
        ss << " -" << i.second.key();
    }
    ss << std::endl << std::endl;

    for (const auto &i : m_Items)
    {
        std::string tmp = "-" + i.second.key() + " [ --" + i.second.lkey() + " ] ";
        ss << "\t" << std::setw(24) << std::left << tmp
           << ": " << i.second.desc() << std::endl;
    }

    return ss.str();
}

void Options::dump() const noexcept
{
    if (get<int>("loglevel") == 0)
        return;

    std::stringbuf str;
    std::ostream os(&str);

    os << "Options dump: " << std::endl;
    for (const auto &i : m_Items)
    {
        os << "\t" << std::setw(16) << std::left << i.second.lkey();
        os << ": " << i.second.value() << std::endl;
    }
    logd1(str.str());
}

void Options::parse(int count, const char *const *args)
{
    int counter = 1;

    while (true)
    {
        if (counter >= count) break;

        std::string tmp(args[counter]);
        int hyphen_counter = 0;
        while (tmp.size() && tmp[hyphen_counter] == '-')
        {
            ++hyphen_counter;
        }

        if (hyphen_counter == 0 || hyphen_counter > 2)
        {
            loge("wrong key: ", args[counter]);
            logi(usage(args[0]));
            exit(-1);
        }

        std::string key = tmp.substr(hyphen_counter, std::string::npos);
        if (key.size() > 1 && m_LongKeys.find(key) == m_LongKeys.end())
        {
            loge("wrong key: ", args[counter]);
            logi(usage(args[0]));
            exit(-1);
        }

        if (hyphen_counter == 1)
        {
            key = tmp[1];
        }
        else
        {
            key = tmp[2];
        }

        auto it = m_Items.find(key);
        if (it == m_Items.end())
        {
            loge("wrong key: ", args[counter]);
            logi(usage(args[0]));
            exit(-1);
        }

        counter = it->second.parseFromProgrammOptions(counter, count, args);
    }

    if (get<bool>("help"))
    {
        logi(usage(args[0]));
        exit(0);
    }
}
