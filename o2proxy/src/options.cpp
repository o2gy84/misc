#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "options.hpp"
#include "utils.hpp"
#include "logger.hpp"


const SettingItem& Options::find_option_by_long_key(const std::string &lk) const throw (std::exception)
{
    if (lk.empty())
    {
        throw std::runtime_error("request for empty option");
    }

    auto it = m_Items.find(lk);
    if (it == m_Items.end())
    {
        throw std::runtime_error("no such option: " + lk);
    }

    return it->second;
}

const SettingItem& Options::find_option_by_short_key(const std::string &k) const throw (std::exception)
{
    if (k.empty())
    {
        throw std::runtime_error("request for empty option");
    }

    for (auto it = m_Items.begin(); it != m_Items.end(); ++it)
    {
        const SettingItem &item = it->second;
        if (item.key() == k)
        {
            return item;
        }
    }

    throw std::runtime_error("no such option: " + k);
}


std::string Options::usage(const std::string &progname) const
{
    // TODO: for more beautifullity, first print short options,
    //       second - long options

    std::stringstream ss;
    ss << "Usage: " << progname;

    for (const auto &i : m_Items)
    {
        const SettingItem &item = i.second;
        if (item.key().empty())
        {
            ss << " --" << item.lkey();
        }
        else
        {
            ss << " -" << item.key();
        }
    }
    ss << std::endl << std::endl;

    for (const auto &i : m_Items)
    {
        const SettingItem &item = i.second;
        std::string desc;
        if (item.key().empty())
        {
            desc = "--" + item.lkey();
        }
        else
        {
            desc = "-" + item.key() + " [ --" + item.lkey() + " ] ";
        }

        ss << "\t" << std::setw(24) << std::left << desc
           << ": " << item.desc() << std::endl;
    }

    return ss.str();
}

void Options::dump() const
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

        if (hyphen_counter == 1)    // short key
        {
            try
            {
                const SettingItem &item = find_option_by_short_key(key);
                key = item.lkey();
            }
            catch (...)
            {
                loge("wrong key: ", args[counter]);
                logi(usage(args[0]));
                exit(-1);
            }
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
