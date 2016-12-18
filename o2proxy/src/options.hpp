#pragma once

#include <map>
#include <set>

#include "settings.hpp"

/*
    Programm option class.
*/
class Options
{
public:
    Options() {};
    void parse(int count, const char *const *args);
    void dump() const;

    template <typename T>
    void add(const std::string &lk, const std::string &k, const std::string &desc, T&& default_value)
    {
        if (lk.empty())
        {
            throw std::runtime_error("key is empty: " + k);
        }
        if (m_Items.find(lk) != m_Items.end())
        {
            throw std::runtime_error("options key already registered: " + lk);
        }

        m_Items[lk] = SettingItem(lk, k, desc, std::forward<T>(default_value));
    }

    template <typename T>
    T get(const std::string &key) const
    {
        const SettingItem &item = find_option_by_long_key(key);
        return item.value().get<T>();
    }

private:
    const SettingItem& find_option_by_long_key(const std::string &lk) const throw (std::exception);
    const SettingItem& find_option_by_short_key(const std::string &k) const throw (std::exception);

    std::string usage(const std::string &program) const;

private:
    std::map<std::string, SettingItem> m_Items; // long key and Item
};
