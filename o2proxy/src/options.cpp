#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "options.hpp"
#include "utils.hpp"
#include "logger.hpp"


std::string Options::usage(const std::string &progname) const
{
    // TODO: for more beautifullity, first print short options,
    //       second - long options

    std::stringstream ss;
    ss << "Usage: " << progname;

    for (const auto &i : m_Storage.items())
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

    for (const auto &i : m_Storage.items())
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
    for (const auto &i : m_Storage.items())
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
                const SettingItem &item = m_Storage.find_option_by_short_key(key);
                key = item.lkey();
            }
            catch (...)
            {
                loge("wrong key: ", args[counter]);
                logi(usage(args[0]));
                exit(-1);
            }
        }

        std::pair<SettingItem &, bool> item = m_Storage.find_option_by_long_key(key);
        if (!item.second)
        {
            loge("wrong key: ", args[counter]);
            logi(usage(args[0]));
            exit(-1);
        }

        counter = parseFromProgrammOptions(item.first, counter, count, args);
    }

    if (get<bool>("help"))
    {
        logi(usage(args[0]));
        exit(0);
    }
}

int Options::parseFromProgrammOptions(SettingItem &item, int cur_counter, int total_opts, const char *const *args)
{
    if (item.value().type() == AnyItem::BOOL)
    {
        // positional option - just yes or no
        item.value().store(true);
        return cur_counter + 1;
    }
    else if (item.value().type() == AnyItem::INT)
    {
        // TODO: parse more complex
        // examples: "-l 5", "-l=5", "-l5"
        if (cur_counter + 1 >= total_opts)
        {
            throw std::runtime_error("bad options");
        }

        int v = std::stoi(args[cur_counter + 1]);
        item.value().store(v);
        return cur_counter + 2;
    }
    else if (item.value().type() == AnyItem::STRING)
    {
        // TODO: parse more complex
        // examples: "-c /etc/config.conf", "-c = /etc/config.conf"
        if (cur_counter + 1 >= total_opts)
        {
            throw std::runtime_error("bad options");
        }

        item.value().store(args[cur_counter + 1]);
        return cur_counter + 2;
    }

    throw std::runtime_error("bad options");
    return -1;
}
